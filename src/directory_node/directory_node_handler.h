//
// Created by masi on 8/29/23.
//

#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/utils/error.h"
#include "common/utils/protocol_handler.h"
#include "directory_store.h"
#include "common/utils/worker_utils.h"

namespace uh::cluster {

    class directory_handler: public protocol_handler {
    public:

        directory_handler(directory_node_config conf, global_data_view &storage, std::shared_ptr <boost::asio::thread_pool> directory_workers) :
                protocol_handler(conf.server_conf),
                m_directory(conf.directory_conf),
                m_storage(storage),
                m_directory_workers (std::move (directory_workers)),
                m_counters(add_counter_family("uh_dr_requests", "number of requests handled by the directory node")),
                m_reqs_dir_put_obj(m_counters.Add({{"type", "DIR_PUT_OBJ_REQ"}})),
                m_reqs_dir_get_obj(m_counters.Add({{"type", "DIR_GET_OBJ_REQ"}})),
                m_reqs_dir_put_bucket(m_counters.Add({{"type", "DIR_PUT_BUCKET_REQ"}})),
                m_reqs_dir_recover(m_counters.Add({{"type", "RECOVER_REQ"}})),
                m_reqs_dir_list_bucket(m_counters.Add({{"type", "DIR_LIST_BUCKET_REQ"}})),
                m_reqs_dir_list_obj(m_counters.Add({{"type", "DIR_LIST_OBJ_REQ"}})),
                m_reqs_dir_delete_bucket(m_counters.Add({{"type", "DIR_DELETE_BUCKET_REQ"}})),
                m_reqs_invalid(m_counters.Add({{"type", "INVALID"}}))
        {
            init();
        }

    coro <void> handle (messenger m) override {

        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header ();
                switch (message_header.type) {
                case DIR_PUT_OBJ_REQ:
                    m_reqs_dir_put_obj.Increment();
                    co_await handle_put_obj (m, message_header);
                    break;
                case DIR_GET_OBJ_REQ:
                    m_reqs_dir_get_obj.Increment();
                    co_await handle_get_obj (m, message_header);
                    break;
                case DIR_PUT_BUCKET_REQ:
                    m_reqs_dir_put_bucket.Increment();
                    co_await handle_put_bucket (m, message_header);
                    break;
                case RECOVER_REQ:
                    m_reqs_dir_recover.Increment();
                    co_await handle_recovery (m, message_header);
                    break;
                case DIR_LIST_BUCKET_REQ:
                    m_reqs_dir_list_bucket.Increment();
                    co_await handle_list_buckets(m, message_header);
                    break;
                case DIR_LIST_OBJ_REQ:
                    m_reqs_dir_list_obj.Increment();
                    co_await handle_list_objects(m, message_header);
                    break;
                case DIR_DELETE_BUCKET_REQ:
                    m_reqs_dir_delete_bucket.Increment();
                    co_await handle_delete_bucket(m, message_header);
                    break;
                case DIR_DELETE_OBJ_REQ:
                    co_await handle_delete_object(m, message_header);
                    break;
                case DIR_BUCKET_EXISTS:
                    co_await handle_bucket_exists(m, message_header);
                    break;
                case STOP:
                    co_return;
                default:
                    m_reqs_invalid.Increment();
                    throw std::invalid_argument ("Invalid message type!");
                }
            } catch (const error_exception& e) {
                err = e.error();
            } catch (const std::exception& e) {
                err = error(error::unknown, e.what());
            }

            if (err)
            {
                co_await m.send_error (*err);
            }
        }

    }

    private:
      
        coro <void> handle_bucket_exists (messenger& m, const messenger::header& h)
        {
            directory_message request = co_await m.recv_directory_message (h);

            m_directory.bucket_exists(request.bucket_id);

            co_await m.send(SUCCESS, {});
            co_return;
        }

        coro <void> handle_put_obj (messenger& m, const messenger::header& h)
        {
            directory_message request = co_await m.recv_directory_message (h);

            auto func = [] (directory_store& directory, const directory_message& request) {
                std::vector<char> address_data;
                zpp::bits::out{address_data, zpp::bits::size4b{}}(*request.addr).or_throw();
                directory.insert (request.bucket_id, *request.object_key, address_data);
            };
            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front (func, std::ref (m_directory), std::cref (request)));

            co_await m.send(SUCCESS, {});
            co_return;
        }

        coro <void> handle_get_obj (messenger& m, const messenger::header& h) {

            directory_message request = co_await m.recv_directory_message (h);

            unique_buffer <char> buffer;

            auto func = [](directory_store& directory, global_data_view& storage, const directory_message& request, unique_buffer <char>& buffer) {
                address addr;
                const auto buf = directory.get(request.bucket_id, *request.object_key);
                zpp::bits::in{buf.get_span(), zpp::bits::size4b{}}(addr).or_throw();
                std::size_t buffer_size = 0;
                for(auto frag_size : addr.sizes){
                    buffer_size += frag_size;
                }
                buffer = unique_buffer <char> (buffer_size);
                storage.read_address(buffer.data(), addr);
            };

            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front(func, std::ref (m_directory), std::ref (m_storage), std::cref(request), std::ref((buffer))));

            m.register_write_buffer(buffer);
            co_await m.send_buffers(DIR_GET_OBJ_RESP);

        }

        coro <void> handle_put_bucket (messenger& m, const messenger::header& h) {
            directory_message request = co_await m.recv_directory_message (h);
            auto func = [] (directory_store& directory, const directory_message& request) {
                directory.add_bucket(request.bucket_id);
            };
            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front(func, std::ref (m_directory), std::cref (request)));
            co_await m.send(SUCCESS, {});

        }

        coro <void> handle_delete_bucket (messenger& m, const messenger::header& h) {
            directory_message request = co_await m.recv_directory_message (h);
            auto func = [] (directory_store& directory, const directory_message& request) {
                directory.remove_bucket(request.bucket_id);
            };
            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front(func, std::ref (m_directory), std::cref (request)));
            co_await m.send(SUCCESS, {});
        }

        coro <void> handle_delete_object (messenger& m, const messenger::header& h) {
            directory_message request = co_await m.recv_directory_message (h);
            auto func = [] (directory_store& directory, const directory_message& request) {
                directory.remove_object(request.bucket_id, *request.object_key);
            };
            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front(func, std::ref (m_directory), std::cref (request)));
            co_await m.send(SUCCESS, {});
        }

        coro <void> handle_list_buckets (messenger& m, const messenger::header &h) {
            directory_lst_entities_message response;

            auto func = [] (directory_store& directory, directory_lst_entities_message& response) {
                response.entities = directory.list_buckets();
            };
            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front(func, std::ref (m_directory), std::ref (response)));
            co_await m.send_directory_list_entities_message(DIR_LIST_BUCKET_RESP, response);
        }

        coro <void> handle_list_objects (messenger& m, const messenger::header &h) {
            directory_message request = co_await m.recv_directory_message (h);
            directory_lst_entities_message response;
            auto func = [] (directory_store& directory, directory_lst_entities_message& response, directory_message& request) {
                response.entities = directory.list_keys(request.bucket_id);
            };
            co_await worker_utils::post_in_workers (*m_directory_workers, *m_storage.get_executor(), std::bind_front(func, std::ref (m_directory), std::ref(response), std::ref(request)));
            co_await m.send_directory_list_entities_message(DIR_LIST_OBJ_RESP, response);
        }

        coro <void> handle_recovery (messenger& m, const messenger::header& h) {
            co_await m.send(RECOVER_RESP, {});
        }

        directory_store m_directory;
        global_data_view& m_storage;
        std::shared_ptr <boost::asio::thread_pool> m_directory_workers;
        prometheus::Family<prometheus::Counter> &m_counters;
        prometheus::Counter &m_reqs_invalid;
        prometheus::Counter &m_reqs_dir_put_obj;
        prometheus::Counter &m_reqs_dir_get_obj;
        prometheus::Counter &m_reqs_dir_put_bucket;
        prometheus::Counter &m_reqs_dir_recover;
        prometheus::Counter &m_reqs_dir_list_bucket;
        prometheus::Counter &m_reqs_dir_list_obj;
        prometheus::Counter &m_reqs_dir_delete_bucket;

    };
} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_HANDLER_H