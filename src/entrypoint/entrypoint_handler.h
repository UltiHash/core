#ifndef ENTRYPOINT_HANDLER_H
#define ENTRYPOINT_HANDLER_H

#include "common/registry/services.h"
#include "common/utils/error.h"
#include "common/utils/protocol_handler.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/http_response.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"
#include "entrypoint/rest/http/models/init_multi_part_upload_response.h"
#include "entrypoint/rest/http/models/list_multi_part_uploads_response.h"
#include "entrypoint/rest/utils/parser/s3_parser.h"
#include "entrypoint/rest/utils/parser/xml_parser.h"
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <pugixml.hpp>

// REFACTORED
#include "requests/abort_multipart.h"
#include "requests/complete_multipart.h"
#include "requests/create_bucket.h"
#include "requests/delete_bucket.h"
#include "requests/delete_object.h"
#include "requests/delete_objects.h"
#include "requests/get_bucket.h"
#include "requests/get_object.h"
#include "requests/get_object_attributes.h"
#include "requests/list_buckets.h"
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
        : m_state(state), m_ioc(state.ioc), m_workers(state.workers),
          m_directory_services(state.directory_services),
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

                bool fallback = false;
                try {
                    http_request req(received_request, s, buffer);
                    auto resp = co_await handle_request(req);
                    co_await boost::beast::http::async_write(
                        s, resp.get_prepared_response(),
                        boost::asio::use_awaitable);
                } catch (const command_unknown_exception&) {
                    fallback = true;
                }

                if (fallback) {
                    uh::cluster::rest::utils::parser::s3_parser s3_parser(
                        received_request, m_state.server_state);
                    auto s3_request = s3_parser.parse();

                    co_await s3_request->read_body(s, buffer);
                    s3_request->validate_request_specific_criteria();

                    auto s3_res = co_await handle_request(*s3_request,
                                                          m_state.server_state);
                    auto s3_res_specific_object =
                        s3_res->get_response_specific_object();
                    co_await boost::beast::http::async_write(
                        s, s3_res_specific_object, boost::asio::use_awaitable);
                    LOG_DEBUG()
                        << "sent response: " << s3_res_specific_object.base();
                }

                if (!received_request.keep_alive()) {
                    break;
                }
            }

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
        } catch (
            uh::cluster::rest::http::model::custom_error_response_exception&
                res_exc) {
            LOG_ERROR() << res_exc.what();
            boost::beast::http::write(s,
                                      res_exc.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        } catch (const std::exception& e) {
            LOG_ERROR() << e.what();
            uh::cluster::rest::http::model::custom_error_response_exception err(
                boost::beast::http::status::internal_server_error);
            boost::beast::http::write(s, err.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        }

        s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        s.close();
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

    coro<std::unique_ptr<rest::http::http_response>>
    handle_request(rest::http::http_request& req,
                   rest::utils::server_state& state) {

        std::unique_ptr<rest::http::http_response> res;

        switch (req.get_request_name()) {
        case rest::http::http_request_type::INIT_MULTIPART_UPLOAD:
            res = co_await handle_init_mp_upload(req, state);
            break;
        case rest::http::http_request_type::LIST_MULTI_PART_UPLOADS:
            res = co_await handle_list_mp_uploads(req, state);
            break;
        default:
            throw std::runtime_error(
                "request not supported by the backend yet.");
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<rest::http::http_response>>
    handle_init_mp_upload(const rest::http::http_request& req,
                          rest::utils::server_state& state) {
        std::unique_ptr<rest::http::model::init_multi_part_upload_response> res;
        try {
            res = std::make_unique<
                rest::http::model::init_multi_part_upload_response>(req);

            co_await worker_utils::
                io_thread_acquire_messenger_and_post_in_io_threads(
                    m_workers, m_ioc, m_directory_services.get(),
                    [&res, &req](client::acquired_messenger m) -> coro<void> {
                        directory_message dir_req{
                            .bucket_id = req.get_URI().get_bucket_id()};

                        co_await m.get().send_directory_message(
                            DIR_BUCKET_EXISTS, dir_req);
                        co_await m.get().recv_header();

                        res->set_upload_id(req.get_eTag());
                    });

        }

        catch (const error_exception& e) {
            switch (*e.error()) {
            case error::bucket_not_found:
                throw rest::http::model::custom_error_response_exception(
                    boost::beast::http::status::not_found,
                    rest::http::model::error::bucket_not_found);
            default:
                throw rest::http::model::custom_error_response_exception(
                    boost::beast::http::status::internal_server_error);
            }
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<rest::http::http_response>>
    handle_list_mp_uploads(const rest::http::http_request& req,
                           rest::utils::server_state& state) {

        std::unique_ptr<rest::http::model::list_multi_part_uploads_response>
            res = std::make_unique<
                rest::http::model::list_multi_part_uploads_response>(req);

        auto func =
            [](std::unique_ptr<
                   rest::http::model::list_multi_part_uploads_response>& res,
               rest::utils::server_state& state,
               const rest::http::http_request& req) {
                auto bucket_name = req.get_URI().get_bucket_id();
                auto multipart_map =
                    state.m_uploads.list_multipart_uploads(bucket_name);

                if (multipart_map.empty()) {
                    throw rest::http::model::custom_error_response_exception(
                        boost::beast::http::status::not_found,
                        rest::http::model::error::bucket_not_found);
                } else {
                    for (const auto& pair : multipart_map) {
                        res->add_key_and_uploadid(pair.first, pair.second);
                    }
                }
            };

        co_await worker_utils::post_in_workers(
            m_workers, m_ioc,
            std::bind_front(func, std::ref(res), std::ref(state),
                            std::cref(req)));

        co_return std::move(res);
    }

  private:
    entrypoint_state& m_state;
    boost::asio::io_context& m_ioc;
    boost::asio::thread_pool& m_workers;
    const services<DIRECTORY_SERVICE>& m_directory_services;
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
        get_object_attributes(state), list_objects(state),
        list_objects_v2(state), delete_object(state), delete_objects(state),
        multipart(state), complete_multipart(state), abort_multipart(state));
}

} // end namespace uh::cluster

#endif // ENTRYPOINT_HANDLER_H
