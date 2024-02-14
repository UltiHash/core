#include "create_bucket.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

create_bucket::create_bucket(uh::cluster::entrypoint_state&& entry_state)
    : m_state(entry_state) {}

bool create_bucket::can_handle(const http_request& req) {

    if (req.get_method() == method::put) {
        const auto& uri = req.get_URI();

        if (!uri.get_bucket_id().empty() && uri.get_object_key().empty() &&
            uri.get_query_parameters().empty()) {
            return true;
        }
    }

    return false;
}

coro<http_response> create_bucket::handle(const http_request& req) const {

    auto bucket_id = req.get_URI().get_bucket_id();
    try {
        auto func = [&bucket_id](const auto& bucket,
                                 client::acquired_messenger m,
                                 long id) -> coro<void> {
            directory_message dir_req{.bucket_id = bucket_id};
            co_await m.get().send_directory_message(DIR_PUT_BUCKET_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };
        co_await worker_utils::broadcast_from_io_thread_in_io_threads(
            m_state.directory_services.get_clients(), m_state.ioc,
            m_state.workers, std::bind_front(func, std::cref(bucket_id)));

        co_return http_response();
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to add the bucket " << bucket_id
                    << " to the directory: " << e;
        throw rest::http::model::custom_error_response_exception(
            boost::beast::http::status::not_found);
    }
}

} // namespace uh::cluster
