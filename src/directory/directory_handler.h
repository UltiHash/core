#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/telemetry/log.h"
#include "common/utils/error.h"
#include "common/utils/protocol_handler.h"
#include "directory/interfaces/directory_interface.h"
#include "directory/interfaces/local_directory.h"
#include "directory_store.h"

#include <fstream>

namespace uh::cluster {

class directory_handler : public protocol_handler {
public:
    explicit directory_handler(local_directory& ldirectory)
        : m_directory(ldirectory) {}

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
            } catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::eof) {
                    LOG_INFO() << remote.str() << " disconnected";
                    break;
                }
                err = error(error::unknown, e.what());
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
    coro<void> handle_bucket_exists(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);

        co_await m_directory.bucket_exists(request.bucket_id);

        co_await m.send(SUCCESS, {});
        co_return;
    }

    coro<void> handle_put_obj(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        co_await m_directory.put_object(request.bucket_id, *request.object_key,
                                        std::move(*request.addr));
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_get_obj(messenger& m, const messenger::header& h) {

        directory_message request = co_await m.recv_directory_message(h);

        auto read_handler = co_await m_directory.get_object(
            request.bucket_id, *request.object_key);
        std::shared_ptr<awaitable_promise<void>> pr;

        do {
            auto data = co_await read_handler->next();
            if (pr) {
                co_await pr->get();
            }
            pr = std::make_shared<awaitable_promise<void>>(
                m_directory.get_executor());
            boost::asio::co_spawn(
                m_directory.get_executor(),
                m.send_directory_get_object_chunk(read_handler->has_next(),
                                                  std::move(data)),
                use_awaitable_promise_cospawn(pr));
        } while (read_handler->has_next());

        co_await pr->get();
    }

    coro<void> handle_put_bucket(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        co_await m_directory.put_bucket(request.bucket_id);
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_delete_bucket(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        co_await m_directory.delete_bucket(request.bucket_id);
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_delete_object(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);
        co_await m_directory.delete_object(request.bucket_id,
                                           *request.object_key);
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_list_buckets(messenger& m, const messenger::header& h) {
        directory_list_buckets_message response;
        response.entities = co_await m_directory.list_buckets();
        co_await m.send_directory_list_buckets_message(response);
    }

    coro<void> handle_list_objects(messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message(h);

        std::optional<std::string> lower_bound, prefix;
        if (request.object_key_lower_bound) {
            lower_bound = *request.object_key_lower_bound;
        }
        if (request.object_key_prefix) {
            prefix = *request.object_key_prefix;
        }

        auto objs = co_await m_directory.list_objects(request.bucket_id, prefix,
                                                      lower_bound);

        directory_list_objects_message response{.objects = std::move(objs)};

        co_await m.send_directory_list_objects_message(response);
    }

    local_directory& m_directory;
};
} // end namespace uh::cluster

#endif // CORE_DIRECTORY_NODE_HANDLER_H
