#include "create_bucket.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

create_bucket::create_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool create_bucket::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::put && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

coro<http_response> create_bucket::handle(const http_request& req) const {
    metric<entrypoint_create_bucket>::increase(1);
    auto bucket_id = req.get_uri().get_bucket_id();
    try {
        auto func = [&bucket_id](const auto& bucket,
                                 client::acquired_messenger m,
                                 long id) -> coro<void> {
            directory_message dir_req{.bucket_id = bucket_id};
            co_await m.get().send_directory_message(DIRECTORY_BUCKET_PUT_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };
        co_await worker_utils::broadcast_from_io_thread_in_io_threads(
            m_collection.directory_services.get_clients(), m_collection.ioc,
            m_collection.workers, std::bind_front(func, std::cref(bucket_id)));

        co_return http_response();
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to add the bucket " << bucket_id
                    << " to the directory: " << e;
        throw command_exception(http::status::not_found);
    }
}

} // namespace uh::cluster
