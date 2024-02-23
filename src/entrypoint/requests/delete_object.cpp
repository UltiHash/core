#include "delete_object.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/custom_error_response_exception.h"

namespace uh::cluster {

delete_object::delete_object(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool delete_object::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::delete_ &&
           !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
           !uri.query_string_exists("uploadId");
}

coro<http_response> delete_object::handle(const http_request& req) const {
    try {
        auto func = [](const http_request& req,
                       client::acquired_messenger m) -> coro<void> {
            directory_message dir_req{
                .bucket_id = req.get_uri().get_bucket_id(),
                .object_key = std::make_unique<std::string>(
                    req.get_uri().get_object_key())};
            co_await m.get().send_directory_message(DIRECTORY_OBJECT_DELETE_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };

        co_await worker_utils::
            io_thread_acquire_messenger_and_post_in_io_threads(
                m_state.workers, m_state.ioc, m_state.directory_services.get(),
                std::bind_front(func, std::cref(req)));

        co_return http_response();
    } catch (const error_exception& e) {
        switch (*e.error()) {
        case error::object_not_found:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::not_found,
                rest::http::model::error::object_not_found);
        case error::bucket_not_found:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::not_found,
                rest::http::model::error::bucket_not_found);
        default:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
