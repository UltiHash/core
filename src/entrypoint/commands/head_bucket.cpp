#include "head_bucket.h"

#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

head_bucket::head_bucket(directory& dir)
    : m_directory(dir) {}

bool head_bucket::can_handle(const http_request& req) {
    return req.method() == method::head &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && !req.query("attributes");
}

coro<http_response> head_bucket::handle(http_request& req) {
    metric<entrypoint_head_object_req>::increase(1);

    try {
        co_await m_directory.bucket_exists(req.bucket());

        co_return http_response{};
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }
}

std::string head_bucket::action_id() const { return "s3:HeadBucket"; }

} // namespace uh::cluster
