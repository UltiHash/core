#include "delete_bucket.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_bucket::delete_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_bucket::can_handle(const http_request& req) {
    return req.method() == method::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
}

coro<void> delete_bucket::handle(http_request& req) const {
    metric<entrypoint_delete_bucket_req>::increase(1);

    try {
        co_await m_collection.directory.delete_bucket(req.bucket());
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    http_response res;
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
