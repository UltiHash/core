#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/telemetry/log.h"
#include "common/utils/error.h"
#include "common/utils/protocol_handler.h"
#include "common/utils/worker_pool.h"
#include "directory_store.h"

#include <fstream>

namespace uh::cluster {

class directory_handler : public protocol_handler {
public:
    // warn about a nearly reached size limit at this percentage
    static constexpr unsigned SIZE_LIMIT_WARNING_PERCENTAGE = 80;
    // number of files to upload between two warnings
    static constexpr unsigned SIZE_LIMIT_WARNING_INTERVAL = 100;

    directory_handler(
        directory_config config, global_data_view& storage,
        worker_pool& directory_workers)
        : m_config(std::move(config)),
          m_directory(m_config.directory_store_conf),
          m_storage(storage),
          m_directory_workers(directory_workers),
          m_stored_size(restore_stored_size()) {
        metric<directory_original_data_volume_gauge, byte, int64_t>::
            register_gauge_callback(
                std::bind(&directory_handler::get_stored_size_64, this));
        metric<directory_deduplicated_data_volume_gauge, byte, int64_t>::
            register_gauge_callback(
                std::bind(&directory_handler::get_effective_size_64, this));
    }

    ~directory_handler() override { persist_stored_size(); }

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        std::stringstream remote;
        remote << s.remote_endpoint();

        messenger m(std::move(s));

        for (;;) {
            std::optional<error> err;

            try {

                const auto message_header = co_await m.recv_header();

                LOG_DEBUG() << remote.str() << " received "
                            << magic_enum::enum_name(message_header.type);

                switch (message_header.type) {
                case DIRECTORY_OBJECT_PUT_REQ:
                    co_await handle_put_obj(m, message_header);
                    break;
                case DIRECTORY_OBJECT_GET_REQ:
                    co_await handle_get_obj(m, message_header);
                    break;
                case DIRECTORY_BUCKET_PUT_REQ:
                    co_await handle_put_bucket(m, message_header);
                    break;
                case DIRECTORY_BUCKET_LIST_REQ:
                    co_await handle_list_buckets(m, message_header);
                    break;
                case DIRECTORY_OBJECT_LIST_REQ:
                    co_await handle_list_objects(m, message_header);
                    break;
                case DIRECTORY_BUCKET_DELETE_REQ:
                    co_await handle_delete_bucket(m, message_header);
                    break;
                case DIRECTORY_OBJECT_DELETE_REQ:
                    co_await handle_delete_object(m, message_header);
                    break;
                case DIRECTORY_BUCKET_EXISTS_REQ:
                    co_await handle_bucket_exists(m, message_header);
                    break;
                default:
                    throw std::invalid_argument("Invalid message type!");
                }
            } catch (const error_exception& e) {
                err = e.error();
            } catch (const std::exception& e) {
                err = error(error::unknown, e.what());
            }

            if (err) {
                LOG_WARN() << remote.str()
                           << " error handling request: " << err->message();
                co_await m.send_error(*err);
            }
        }
    }

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
            for (const auto& key : m_directory.list_keys(bucket)) {
                address addr;
                const auto& data = m_directory.get(bucket, key);
                try {
                    zpp::bits::in{data.get_span(), zpp::bits::size4b{}}(addr)
                        .or_throw();
                    rv += addr.data_size();
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

    coro<void> handle_put_obj(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);

        const auto size = request.addr->data_size();
        check_size_limit(size);

        auto func = [](directory_store& directory,
                       const directory_message& request) {
            std::vector<char> address_data;
            zpp::bits::out{address_data, zpp::bits::size4b{}}(*request.addr)
                .or_throw();
            directory.insert(request.bucket_id, *request.object_key,
                             address_data);
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_get_obj(messenger& m, const messenger::header& h) {

        directory_message request = co_await m.recv_directory_message(h);


        auto func = [download_chunk_size = m_config.download_chunk_size, &m](directory_store& directory, global_data_view& storage,
                       const directory_message& request) {
            address addr;
            const auto buf =
                directory.get(request.bucket_id, *request.object_key);
            zpp::bits::in{buf.get_span(), zpp::bits::size4b{}}(addr).or_throw();

            size_t addr_index = 0;

            std::optional <std::future <void>> send_to_ep;
            while (addr_index < addr.size()) {
                std::size_t buffer_size = 0;
                address partial_addr;
                while (addr_index < addr.size() and buffer_size < download_chunk_size) {
                    const auto frag = addr.get_fragment(addr_index);
                    partial_addr.push_fragment(frag);
                    buffer_size += frag.size;
                    addr_index ++;
                }
                unique_buffer<char> buffer (buffer_size);

                storage.read_address(buffer.data(), partial_addr);

                bool has_next = addr_index != addr.size();

                if (send_to_ep)
                    send_to_ep->get();

                send_to_ep.emplace(boost::asio::co_spawn(storage.get_executor(), m.send_directory_get_object_chunk(has_next, std::move (buffer)), boost::asio::use_future));

            }
            if (send_to_ep)
                send_to_ep->get();
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::ref(m_storage),
                            std::cref(request)));

    }

    coro<void> handle_put_bucket(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        auto func = [](directory_store& directory,
                       const directory_message& request) {
            directory.add_bucket(request.bucket_id);
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_delete_bucket(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        auto func = [](directory_store& directory,
                       const directory_message& request) {
            directory.remove_bucket(request.bucket_id);
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_delete_object(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        auto func = [this](directory_store& directory,
                           const directory_message& request) {
            try {
                address addr;
                auto buf =
                    directory.get(request.bucket_id, *request.object_key);
                zpp::bits::in{buf.get_span(), zpp::bits::size4b{}}(addr)
                    .or_throw();
                const auto size = addr.data_size();
                decrement_stored_size(size);
                directory.remove_object(request.bucket_id, *request.object_key);
            } catch (const std::exception&) {
            }
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_list_buckets(messenger& m, const messenger::header& h) {
        directory_lst_entities_message response;

        auto func = [](directory_store& directory,
                       directory_lst_entities_message& response) {
            response.entities = directory.list_buckets();
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::ref(response)));

        co_await m.send_directory_list_entities_message(response);
    }

    coro<void> handle_list_objects(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        directory_lst_entities_message response;
        auto func = [](directory_store& directory,
                       directory_lst_entities_message& response,
                       directory_message& request) {
            std::string lower_bound, prefix;
            if (request.object_key_lower_bound) {
                lower_bound = *request.object_key_lower_bound;
            }
            if (request.object_key_prefix) {
                prefix = *request.object_key_prefix;
            }
            response.entities =
                directory.list_keys(request.bucket_id, lower_bound, prefix);
        };

        co_await m_directory_workers.post_in_workers(
            std::bind_front(func, std::ref(m_directory), std::ref(response),
                            std::ref(request)));

        co_await m.send_directory_list_entities_message(response);
    }

    const directory_config m_config;
    directory_store m_directory;
    global_data_view& m_storage;
    worker_pool& m_directory_workers;
    std::mutex m_mutex_size;
    uint128_t m_stored_size;
};
} // end namespace uh::cluster

#endif // CORE_DIRECTORY_NODE_HANDLER_H
