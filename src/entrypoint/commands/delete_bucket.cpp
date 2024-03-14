#include "delete_bucket.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_bucket::delete_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_bucket::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::delete_ &&
           !uri.get_bucket_id().empty() && uri.get_object_key().empty() &&
           uri.get_query_parameters().empty();
}

coro<void> delete_bucket::handle(http_request& req) const {
    metric<entrypoint_delete_bucket_req>::increase(1);
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
        co_await m_collection.workers.broadcast_from_io_thread_in_io_threads(
            m_collection.directory_services.get_clients(),
            std::bind_front(func, std::cref(bucket_name)));

        http_response res;
        co_await req.respond(res.get_prepared_response());
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to delete bucket: " << e;
        switch (*e.error()) {
        case error::bucket_not_found:
            throw command_exception(http::status::not_found,
                                    command_error::bucket_not_found);
        case error::bucket_not_empty:
            throw command_exception(http::status::conflict,
                                    command_error::bucket_not_empty);
        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
