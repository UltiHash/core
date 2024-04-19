#ifndef ENTRYPOINT_HANDLER_H
#define ENTRYPOINT_HANDLER_H

#include "common/registry/services.h"
#include "common/utils/class_name.h"
#include "common/utils/protocol_handler.h"
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>

#include "commands/abort_multipart.h"
#include "commands/complete_multipart.h"
#include "commands/create_bucket.h"
#include "commands/delete_bucket.h"
#include "commands/delete_object.h"
#include "commands/delete_objects.h"
#include "commands/get_bucket.h"
#include "commands/get_object.h"
#include "commands/head_object.h"
#include "commands/init_multipart.h"
#include "commands/list_buckets.h"
#include "commands/list_multipart.h"
#include "commands/list_objects.h"
#include "commands/list_objects_v2.h"
#include "commands/multipart.h"
#include "commands/put_object.h"
#include "http/command_exception.h"

namespace uh::cluster {

template <typename... RequestTypes>
class entrypoint_handler : public protocol_handler {
public:
    explicit entrypoint_handler(reference_collection& collection,
                                RequestTypes&&... request_types)
        : m_collection(collection),
          m_req_types(request_types...) {}

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        try {

            for (;;) {

                auto req = co_await http_request::create(s);
                LOG_DEBUG() << s.remote_endpoint() << " read request: " << *req;

                co_await handle_request(*req);
                metric<success>::increase(1);

                if (!req->keep_alive()) {
                    break;
                }
            }
        } catch (command_exception& res_exc) {
            LOG_ERROR() << res_exc.what();
            http::write(s, res_exc.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        } catch (const boost::system::system_error& se) {
            if (se.code() != http::error::end_of_stream) {
                LOG_ERROR() << se.what();
                command_exception err(http::status::bad_request);
                http::write(s, err.get_response_specific_object());
                s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                s.close();
                throw;
            }
        } catch (const std::invalid_argument& e) {
            LOG_ERROR() << e.what();
            command_exception err(http::status::bad_request,
                                  command_error::invalid_argument);
            http::write(s, err.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        } catch (const std::exception& e) {
            LOG_ERROR() << e.what();
            command_exception err(http::status::internal_server_error);
            http::write(s, err.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        }

        s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        s.close();
    }

    coro<void> handle_request(http_request& req) {
        return dispatch_unpack_tuple(
            req, m_req_types,
            std::make_index_sequence<
                std::tuple_size_v<decltype(m_req_types)>>());
    }

    template <std::size_t... Is>
    coro<void> dispatch_unpack_tuple(http_request& req, auto&& req_types,
                                     std::index_sequence<Is...>) {
        return dispatch_front(req, std::get<Is>(req_types)...);
    }

    template <typename command, typename... commands>
    coro<void> dispatch_front(http_request& req, command&& head,
                              commands&&... tail) {
        if (head.can_handle(req)) {
            LOG_DEBUG() << req.socket().remote_endpoint()
                        << " handling request " << class_name<command>();
            return head.handle(req);
        }
        return dispatch_front(req, std::forward<commands>(tail)...);
    }

    coro<void> dispatch_front(const http_request& req) {
        throw command_exception(http::status::not_found,
                                command_error::command_not_found);
    }

private:
    reference_collection& m_collection;
    std::tuple<RequestTypes...> m_req_types;
};

template <typename... RequestTypes>
auto define_entrypoint_handler(reference_collection& collection,
                               RequestTypes&&... request_types) {
    return std::make_unique<entrypoint_handler<RequestTypes...>>(
        collection, std::forward<RequestTypes>(request_types)...);
}

auto make_entrypoint_handler(reference_collection& collection) {
    return define_entrypoint_handler(
        collection, create_bucket(collection), get_bucket(collection),
        list_buckets(collection), delete_bucket(collection),
        put_object(collection), get_object(collection), head_object(collection),
        list_objects(collection), list_objects_v2(collection),
        delete_object(collection), delete_objects(collection),
        init_multipart(collection), multipart(collection),
        complete_multipart(collection), abort_multipart(collection),
        list_multipart(collection));
}

} // end namespace uh::cluster

#endif // ENTRYPOINT_HANDLER_H
