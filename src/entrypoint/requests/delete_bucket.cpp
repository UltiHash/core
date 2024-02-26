#include "delete_bucket.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

delete_bucket::delete_bucket(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool delete_bucket::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::delete_ &&
           !uri.get_bucket_id().empty() && uri.get_object_key().empty() &&
           uri.get_query_parameters().empty();
}

coro<http_response> delete_bucket::handle(const http_request& req) const {
    metric<entrypoint_delete_bucket>::increase(1);
    try {
        std::string bucket_name = req.get_uri().get_bucket_id();

        auto func = [](const std::string& bucket_name,
                       client::acquired_messenger m, long id) -> coro<void> {
            directory_message dir_req;
            dir_req.bucket_id = bucket_name;
            co_await m.get().send_directory_message(DIRECTORY_BUCKET_DELETE_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };
        co_await worker_utils::broadcast_from_io_thread_in_io_threads(
            m_state.directory_services.get_clients(), m_state.ioc,
            m_state.workers, std::bind_front(func, std::cref(bucket_name)));

        co_return http_response();
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to delete bucket: " << e;
        switch (*e.error()) {
        case error::bucket_not_found:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::not_found,
                rest::http::model::error::bucket_not_found);
        case error::bucket_not_empty:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::conflict,
                rest::http::model::error::bucket_not_empty);
        default:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
