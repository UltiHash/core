#ifndef ENTRYPOINT_HANDLER_H
#define ENTRYPOINT_HANDLER_H

#include "common/registry/services.h"
#include "common/utils/protocol_handler.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>

// REQUESTS
#include "requests/abort_multipart.h"
#include "requests/complete_multipart.h"
#include "requests/create_bucket.h"
#include "requests/delete_bucket.h"
#include "requests/delete_object.h"
#include "requests/delete_objects.h"
#include "requests/get_bucket.h"
#include "requests/get_object.h"
#include "requests/init_multipart.h"
#include "requests/list_buckets.h"
#include "requests/list_multipart.h"
#include "requests/list_objects.h"
#include "requests/list_objects_v2.h"
#include "requests/multipart.h"
#include "requests/put_object.h"

namespace uh::cluster {

template <typename... RequestTypes>
class entrypoint_handler : public protocol_handler {
public:
    explicit entrypoint_handler(entrypoint_state& state,
                                RequestTypes&&... request_types)
        : m_state(state),
          m_req_types(request_types...) {}

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        LOG_INFO() << "connection from: " << s.remote_endpoint();

        try {

            for (;;) {

                boost::beast::flat_buffer buffer;

                boost::beast::http::request_parser<
                    boost::beast::http::empty_body>
                    received_request;
                received_request.body_limit(
                    (std::numeric_limits<std::uint64_t>::max)());

                co_await boost::beast::http::async_read_header(
                    s, buffer, received_request, boost::asio::use_awaitable);
                LOG_DEBUG()
                    << "received request: " << received_request.get().base();

                http_request req(received_request, s, buffer);
                auto resp = co_await handle_request(req);
                co_await boost::beast::http::async_write(
                    s, resp.get_prepared_response(),
                    boost::asio::use_awaitable);

                if (!received_request.keep_alive()) {
                    break;
                }

                s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                s.close();
            }
        } catch (const command_unknown_exception& e) {
            LOG_ERROR() << e.what();
            uh::cluster::rest::http::model::custom_error_response_exception err(
                boost::beast::http::status::not_found,
                rest::http::model::error::command_not_found);
            boost::beast::http::write(s, err.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        } catch (
            uh::cluster::rest::http::model::custom_error_response_exception&
                res_exc) {
            LOG_ERROR() << res_exc.what();
            boost::beast::http::write(s,
                                      res_exc.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        } catch (const boost::system::system_error& se) {
            if (se.code() != boost::beast::http::error::end_of_stream) {
                LOG_ERROR() << se.what();
                uh::cluster::rest::http::model::custom_error_response_exception
                    err(boost::beast::http::status::bad_request);
                boost::beast::http::write(s,
                                          err.get_response_specific_object());
                s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                s.close();
                throw;
            }
        } catch (const std::exception& e) {
            LOG_ERROR() << e.what();
            uh::cluster::rest::http::model::custom_error_response_exception err(
                boost::beast::http::status::internal_server_error);
            boost::beast::http::write(s, err.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        }
    }

    coro<http_response> handle_request(http_request& req) {
        return dispatch_unpack_tuple(
            req, m_req_types,
            std::make_index_sequence<
                std::tuple_size_v<decltype(m_req_types)>>());
    }

    template <std::size_t... Is>
    coro<http_response> dispatch_unpack_tuple(http_request& req,
                                              auto&& req_types,
                                              std::index_sequence<Is...>) {
        return dispatch_front(req, std::get<Is>(req_types)...);
    }

    template <typename command, typename... commands>
    coro<http_response> dispatch_front(http_request& req, command&& head,
                                       commands&&... tail) {
        if (head.can_handle(req)) {
            return head.handle(req);
        }
        return dispatch_front(req, std::forward<commands>(tail)...);
    }

    coro<http_response> dispatch_front(const http_request& req) {
        throw command_unknown_exception();
    }

private:
    entrypoint_state& m_state;
    std::tuple<RequestTypes...> m_req_types;
};

template <typename... RequestTypes>
auto define_entrypoint_handler(entrypoint_state& state,
                               RequestTypes&&... request_types) {
    return std::make_unique<entrypoint_handler<RequestTypes...>>(
        state, std::forward<RequestTypes>(request_types)...);
}

auto make_entrypoint_handler(entrypoint_state& state) {
    return define_entrypoint_handler(
        state, create_bucket(state), get_bucket(state), list_buckets(state),
        delete_bucket(state), put_object(state), get_object(state),
        list_objects(state), list_objects_v2(state), delete_object(state),
        delete_objects(state), init_multipart(state), multipart(state),
        complete_multipart(state), abort_multipart(state),
        list_multipart(state));
}

} // end namespace uh::cluster

#endif // ENTRYPOINT_HANDLER_H
