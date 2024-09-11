#include "delete_bucket.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_bucket::delete_bucket(directory& dir)
    : m_directory(dir) {}

bool delete_bucket::can_handle(const http_request& req) {
    return req.method() == method::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
}

coro<http_response> delete_bucket::handle(http_request& req) {
    metric<entrypoint_delete_bucket_req>::increase(1);

    try {
        co_await m_directory.delete_bucket(req.bucket());
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to delete the bucket " << req.bucket() << ":"
                    << e;
        throw_from_error(e.error());
    }

    co_return http_response{};
}

std::string delete_bucket::action_id() const { return "s3:DeleteBucket"; }

} // namespace uh::cluster
