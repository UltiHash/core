#include "delete_bucket.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_bucket::delete_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_bucket::can_handle(const http_request& req) {
    return req.method() == method::delete_ && !req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
}

coro<void> delete_bucket::handle(http_request& req) const {
    metric<entrypoint_delete_bucket_req>::increase(1);

    try {
        co_await m_collection.directory.delete_bucket(req.bucket());

        http_response res;
        co_await req.respond(res.get_prepared_response());
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to delete bucket: " << e;
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
