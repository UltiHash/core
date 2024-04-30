
#ifndef UH_CLUSTER_LOCAL_DIRECTORY_H
#define UH_CLUSTER_LOCAL_DIRECTORY_H

#include "common/global_data/global_data_view.h"
#include "common/registry/services.h"
#include "common/utils/common.h"
#include "directory/directory_store.h"
#include "directory_interface.h"

namespace uh::cluster {

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

} // namespace uh::cluster

#endif // UH_CLUSTER_LOCAL_DIRECTORY_H
