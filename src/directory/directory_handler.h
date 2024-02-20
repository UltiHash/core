#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/utils/error.h"
#include "common/utils/protocol_handler.h"
#include "common/utils/worker_utils.h"
#include "directory_store.h"

#include <fstream>

namespace uh::cluster {

class directory_handler : public protocol_handler {
  public:
    directory_handler(
        directory_config config, global_data_view& storage,
        std::shared_ptr<boost::asio::thread_pool> directory_workers,
        const std::string& telemetry_endpoint)
        : m_config(std::move(config)),
          m_directory(m_config.directory_store_conf), m_storage(storage),
          m_directory_workers(std::move(directory_workers)),
          m_stored_size(get_stored_size()),
          m_metrics_handler(telemetry_endpoint) {}

    ~directory_handler() override { write_stored_size(); }

    coro<void> handle(boost::asio::ip::tcp::socket s) override {

        messenger m(std::move(s), m_metrics_handler);

        for (;;) {
            std::optional<error> err;

            try {

                const auto message_header = co_await m.recv_header();
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
                co_await m.send_error(*err);
            }
        }
    }

  private:
    coro<void> handle_bucket_exists(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);

        m_directory.bucket_exists(request.bucket_id);

        co_await m.send(SUCCESS, {});
        co_return;
    }

    void write_stored_size() const {
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
    uint128_t get_stored_size() {
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

    void lower_size_limit(const uint128_t& decrement) {
        std::unique_lock<std::mutex> lk(m_mutex_size);

        m_stored_size = m_stored_size - decrement;
    }

    void check_size_limit(const uint128_t& increment) {
        std::unique_lock<std::mutex> lk(m_mutex_size);

        auto new_size = m_stored_size + increment;

        if (new_size > m_config.max_data_store_size) {
            throw error_exception(error::storage_limit_exceeded);
        }

        m_stored_size = new_size;
    }

    coro<void> handle_put_obj(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);

        check_size_limit(request.addr->data_size());

        auto func = [](directory_store& directory,
                       const directory_message& request) {
            std::vector<char> address_data;
            zpp::bits::out{address_data, zpp::bits::size4b{}}(*request.addr)
                .or_throw();
            directory.insert(request.bucket_id, *request.object_key,
                             address_data);
        };

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
        co_return;
    }

    coro<void> handle_get_obj(messenger& m, const messenger::header& h) {

        directory_message request = co_await m.recv_directory_message(h);

        unique_buffer<char> buffer;

        auto func = [](directory_store& directory, global_data_view& storage,
                       const directory_message& request,
                       unique_buffer<char>& buffer) {
            address addr;
            const auto buf =
                directory.get(request.bucket_id, *request.object_key);
            zpp::bits::in{buf.get_span(), zpp::bits::size4b{}}(addr).or_throw();
            std::size_t buffer_size = 0;
            for (auto frag_size : addr.sizes) {
                buffer_size += frag_size;
            }
            buffer = unique_buffer<char>(buffer_size);
            storage.read_address(buffer.data(), addr);
        };

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
            std::bind_front(func, std::ref(m_directory), std::ref(m_storage),
                            std::cref(request), std::ref((buffer))));

        m.register_write_buffer(buffer);
        co_await m.send_buffers(SUCCESS);
    }

    coro<void> handle_put_bucket(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        auto func = [](directory_store& directory,
                       const directory_message& request) {
            directory.add_bucket(request.bucket_id);
        };

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_delete_bucket(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        auto func = [](directory_store& directory,
                       const directory_message& request) {
            directory.remove_bucket(request.bucket_id);
        };

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
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
                lower_size_limit(addr.data_size());

                directory.remove_object(request.bucket_id, *request.object_key);
            } catch (const std::exception&) {
            }
        };

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
            std::bind_front(func, std::ref(m_directory), std::cref(request)));

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_list_buckets(messenger& m, const messenger::header& h) {
        directory_lst_entities_message response;

        auto func = [](directory_store& directory,
                       directory_lst_entities_message& response) {
            response.entities = directory.list_buckets();
        };

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
            std::bind_front(func, std::ref(m_directory), std::ref(response)));

        co_await m.send_directory_list_entities_message(SUCCESS, response);
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

        co_await worker_utils::post_in_workers(
            *m_directory_workers, m_storage.get_executor(),
            std::bind_front(func, std::ref(m_directory), std::ref(response),
                            std::ref(request)));

        co_await m.send_directory_list_entities_message(SUCCESS, response);
    }

    const directory_config m_config;
    directory_store m_directory;
    global_data_view& m_storage;
    std::shared_ptr<boost::asio::thread_pool> m_directory_workers;
    std::mutex m_mutex_size;
    uint128_t m_stored_size;
    metrics_handler m_metrics_handler;
};
} // end namespace uh::cluster

#endif // CORE_DIRECTORY_NODE_HANDLER_H
