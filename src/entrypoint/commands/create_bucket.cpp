#include "create_bucket.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

create_bucket::create_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool create_bucket::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::put && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

coro<void> create_bucket::handle(http_request& req) const {
    metric<entrypoint_create_bucket_req>::increase(1);
    auto bucket_id = req.get_uri().get_bucket_id();
    try {
        auto func = [&bucket_id](acquired_messenger<coro_client> m,
                                 std::size_t id) -> coro<void> {
            directory_message req{.bucket_id = bucket_id};
            co_await m->send_directory_message(DIRECTORY_BUCKET_PUT_REQ, req);
            co_await m->recv_header();
        };

        co_await m_collection.directory_services.broadcast(func);

        http_response res;
        co_await req.respond(res.get_prepared_response());
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to add the bucket " << bucket_id
                    << " to the directory: " << e;
        switch (*e.error()) {
        case error::bucket_already_exists:
            throw command_exception(http::status::conflict,
                                    command_error::bucket_already_exists);
        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
