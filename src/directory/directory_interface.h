
#ifndef UH_CLUSTER_DIRECTORY_INTERFACE_H
#define UH_CLUSTER_DIRECTORY_INTERFACE_H

#include "common/network/messenger_core.h"
#include "common/registry/services.h"
#include "common/utils/common.h"
#include "common/utils/coro_utils.h"
#include "directory_store.h"

namespace uh::cluster {

struct directory_interface {

    struct read_handle {
        virtual bool has_next() = 0;
        virtual coro<std::string> next() = 0;
        virtual ~read_handle() = default;
    };

    static constexpr role service_role = DIRECTORY_SERVICE;

    virtual coro<void> put_object(const std::string& bucket,
                                  const std::string& object_id,
                                  const address& addr) = 0;
    virtual coro<std::unique_ptr<read_handle>>
    get_object(const std::string& bucket, const std::string& object_id) = 0;
    virtual coro<void> put_bucket(const std::string& bucket) = 0;
    virtual coro<void> bucket_exists(const std::string& bucket) = 0;
    virtual coro<void> delete_bucket(const std::string& bucket) = 0;
    virtual coro<void> delete_object(const std::string& bucket,
                                     const std::string& object_id) = 0;
    virtual coro<std::vector<std::string>> list_buckets() = 0;
    virtual coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound) = 0;

    virtual ~directory_interface() = default;
};

struct local_directory : public directory_interface {

    struct local_read_handle : public read_handle {
        global_data_view& m_storage;
        const address m_addr;
        const size_t m_chunk_size;
        size_t m_addr_index = 0;

        local_read_handle(global_data_view& storage, address&& addr,
                          size_t chunk_size)
            : m_storage(storage),
              m_addr(std::move(addr)),
              m_chunk_size(chunk_size) {}

        bool has_next() override { return m_addr_index != m_addr.size(); }

        coro<std::string> next() override {
            std::size_t buffer_size = 0;
            address partial_addr;
            while (m_addr_index < m_addr.size() and
                   buffer_size < m_chunk_size) {
                const auto frag = m_addr.get_fragment(m_addr_index);
                partial_addr.push_fragment(frag);
                buffer_size += frag.size;
                m_addr_index++;
            }
            std::string buffer(buffer_size, '\0');

            m_storage.read_address(buffer.data(), partial_addr);
            co_return buffer;
        }
    };

    // warn about a nearly reached size limit at this percentage
    static constexpr unsigned SIZE_LIMIT_WARNING_PERCENTAGE = 80;
    // number of files to upload between two warnings
    static constexpr unsigned SIZE_LIMIT_WARNING_INTERVAL = 100;

    local_directory(directory_config config, global_data_view& storage)
        : m_config(std::move(config)),
          m_directory(m_config.directory_store_conf),
          m_storage(storage),
          m_stored_size(restore_stored_size()) {
        metric<directory_original_data_volume_gauge, byte, int64_t>::
            register_gauge_callback(
                std::bind(&local_directory::get_stored_size_64, this));
        metric<directory_deduplicated_data_volume_gauge, byte, int64_t>::
            register_gauge_callback(
                std::bind(&local_directory::get_effective_size_64, this));
    }

    coro<void> put_object(const std::string& bucket,
                          const std::string& object_id,
                          const address& addr) override {
        check_size_limit(addr.data_size());
        m_directory.insert(bucket, object_id, addr);
        co_return;
    }

    coro<std::unique_ptr<read_handle>>
    get_object(const std::string& bucket,
               const std::string& object_id) override {
        auto addr = m_directory.get(bucket, object_id);
        co_return std::make_unique<local_read_handle>(
            m_storage, std::move(addr), m_config.download_chunk_size);
    }

    coro<void> put_bucket(const std::string& bucket) override {
        m_directory.add_bucket(bucket);
        co_return;
    }

    coro<void> bucket_exists(const std::string& bucket) override {
        m_directory.bucket_exists(bucket);
        co_return;
    }

    coro<void> delete_bucket(const std::string& bucket) override {
        m_directory.remove_bucket(bucket);
        co_return;
    }

    coro<void> delete_object(const std::string& bucket,
                             const std::string& object_id) override {
        try {
            const auto addr = m_directory.get(bucket, object_id);
            const auto size = addr.data_size();
            decrement_stored_size(size);
            m_directory.remove_object(bucket, object_id);
        } catch (const std::exception& e) {
            LOG_WARN() << "deletion of " << object_id << " in " << bucket
                       << " failed: " << e.what();
        }
        co_return;
    }

    coro<std::vector<std::string>> list_buckets() override {
        co_return m_directory.list_buckets();
    }

    coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound) override {
        std::string lower_bound_str, prefix_str;
        if (lower_bound) {
            lower_bound_str = *lower_bound;
        }
        if (prefix) {
            prefix_str = *prefix;
        }
        co_return m_directory.list_objects(bucket, lower_bound_str, prefix_str);
    }

    boost::asio::io_context& get_executor() { return m_storage.get_executor(); }

    ~local_directory() override { persist_stored_size(); }

private:
    uint64_t get_stored_size_64() { return m_stored_size.get_low(); }
    uint64_t get_effective_size_64() {
        return m_storage.get_used_space().get_low();
    }

    coro<void> handle_bucket_exists(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);

        m_directory.bucket_exists(request.bucket_id);

        co_await m.send(SUCCESS, {});
        co_return;
    }

    void persist_stored_size() const {
        try {
            std::ofstream out(
                (m_config.directory_store_conf.working_dir / "cache").string());
            out << m_stored_size.to_string();
        } catch (...) {
        }
    }

    /**
     * Return the size of data referenced by the directory.
     *
     * @note The value is cached in the directory's working dir. If the
     * value is not cached, this will traverse all buckets and keys. This is
     * potentially very expensive. As a result this function is private and
     * only called during construction.
     */
    uint128_t restore_stored_size() {
        try {
            std::ifstream in(m_config.directory_store_conf.working_dir /
                             "cache");
            std::string s;
            in >> s;
            return uint128_t(s);
        } catch (const std::exception&) {
        }

        uint128_t rv;
        for (const auto& bucket : m_directory.list_buckets()) {
            for (const auto& obj : m_directory.list_objects(bucket)) {
                const auto address = m_directory.get(bucket, obj.name);
                try {
                    rv += address.data_size();
                } catch (const std::exception& e) {
                }
            }
        }

        return rv;
    }

    void decrement_stored_size(const uint128_t& decrement) {
        std::unique_lock<std::mutex> lk(m_mutex_size);

        m_stored_size = m_stored_size - decrement;
    }

    void check_size_limit(const uint128_t& increment) {
        std::unique_lock<std::mutex> lk(m_mutex_size);

        auto new_size = m_stored_size + increment;

        if (new_size > m_config.max_data_store_size) {
            throw error_exception(error::storage_limit_exceeded);
        }

        static unsigned warn_counter = 0;
        if (new_size * 100 >
            m_config.max_data_store_size * SIZE_LIMIT_WARNING_PERCENTAGE) {
            if (warn_counter == 0) {
                LOG_WARN() << "over " << SIZE_LIMIT_WARNING_PERCENTAGE
                           << "% of storage limit reached";
                warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
            }

            --warn_counter;
        }

        m_stored_size = new_size;
    }

    const directory_config m_config;
    directory_store m_directory;
    global_data_view& m_storage;
    std::mutex m_mutex_size;
    uint128_t m_stored_size;
};

struct remote_directory : public directory_interface {

    struct remote_read_handle : public read_handle {
        acquired_messenger<coro_client> m_messenger;

        bool m_transfer_completed = false;
        remote_read_handle(acquired_messenger<coro_client> m)
            : m_messenger(std::move(m)) {}

        bool has_next() override { return !m_transfer_completed; }

        coro<std::string> next() override {
            auto h = co_await m_messenger->recv_header();
            auto [b_next, buf] =
                co_await m_messenger->recv_directory_get_object_chunk(h);
            m_transfer_completed = !b_next;
            co_return buf;
        }
    };

    explicit remote_directory(coro_client directory_service)
        : m_directory_service(std::move(directory_service)) {}
    coro<void> put_object(const std::string& bucket,
                          const std::string& object_id,
                          const address& addr) override {
        const directory_message dir_req{
            .bucket_id = bucket,
            .object_key = std::make_unique<std::string>(object_id),
            .addr = std::make_unique<address>(addr),
        };
        auto m = co_await m_directory_service.acquire_messenger();
        co_await m->send_directory_message(DIRECTORY_OBJECT_PUT_REQ, dir_req);
        co_await m->recv_header();
    }
    coro<std::unique_ptr<read_handle>>
    get_object(const std::string& bucket,
               const std::string& object_id) override {
        auto m = co_await m_directory_service.acquire_messenger();

        directory_message dir_req;
        dir_req.bucket_id = bucket;
        dir_req.object_key = std::make_unique<std::string>(object_id);

        co_await m->send_directory_message(DIRECTORY_OBJECT_GET_REQ, dir_req);
        co_return std::make_unique<remote_read_handle>(std::move(m));
    }
    coro<void> put_bucket(const std::string& bucket) override {
        auto m = co_await m_directory_service.acquire_messenger();
        directory_message req{.bucket_id = bucket};
        co_await m->send_directory_message(DIRECTORY_BUCKET_PUT_REQ, req);
        co_await m->recv_header();
    }
    coro<void> bucket_exists(const std::string& bucket) override {
        auto m = co_await m_directory_service.acquire_messenger();
        directory_message req{.bucket_id = bucket};
        co_await m->send_directory_message(DIRECTORY_BUCKET_EXISTS_REQ, req);
        co_await m->recv_header();
    }
    coro<void> delete_bucket(const std::string& bucket) override {

        auto m = co_await m_directory_service.acquire_messenger();
        directory_message req;
        req.bucket_id = bucket;
        co_await m->send_directory_message(DIRECTORY_BUCKET_DELETE_REQ, req);
        co_await m->recv_header();
    }
    coro<void> delete_object(const std::string& bucket,
                             const std::string& object_id) override {
        directory_message dir_req{.bucket_id = bucket,
                                  .object_key =
                                      std::make_unique<std::string>(object_id)};
        auto m = co_await m_directory_service.acquire_messenger();

        co_await m->send_directory_message(DIRECTORY_OBJECT_DELETE_REQ,
                                           dir_req);
        co_await m->recv_header();
    }
    coro<std::vector<std::string>> list_buckets() override {
        auto m = co_await m_directory_service.acquire_messenger();

        co_await m->send(DIRECTORY_BUCKET_LIST_REQ, {});

        const auto h = co_await m->recv_header();
        auto result = co_await m->recv_directory_list_buckets_message(h);

        std::vector<std::string> buckets_found;
        for (auto& bucket : result.entities) {
            buckets_found.emplace_back(std::move(bucket));
        }
        co_return buckets_found;
    }
    coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound) override {
        auto m = co_await m_directory_service.acquire_messenger();
        directory_message dir_req{.bucket_id = bucket};

        co_await m->send_directory_message(DIRECTORY_OBJECT_LIST_REQ, dir_req);

        auto h_dir = co_await m->recv_header();
        auto list_objs_res =
            co_await m->recv_directory_list_objects_message(h_dir);
        co_return std::move(list_objs_res.objects);
    }

private:
    coro_client m_directory_service;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_DIRECTORY_INTERFACE_H
