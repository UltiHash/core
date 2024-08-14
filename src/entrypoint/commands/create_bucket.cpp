#include "create_bucket.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

create_bucket::create_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool create_bucket::can_handle(const http_request& req) {
    return req.method() == method::put &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
}

coro<http_response> create_bucket::handle(http_request& req) const {
    metric<entrypoint_create_bucket_req>::increase(1);
    auto bucket_id = req.bucket();
    try {
        co_await m_collection.directory.put_bucket(bucket_id);
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to add the bucket " << bucket_id
                    << " to the directory: " << e;
        throw_from_error(e.error());
    }

    co_return http_response{};
}

} // namespace uh::cluster
