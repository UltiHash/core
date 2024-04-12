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

        auto func = [&bucket_name](acquired_messenger<coro_client> m,
                                   long id) -> coro<void> {
            directory_message req;
            req.bucket_id = bucket_name;
            co_await m->send_directory_message(DIRECTORY_BUCKET_DELETE_REQ,
                                               req);
            co_await m->recv_header();
        };
        co_await m_collection.directory_services.broadcast(func);

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
