#include "delete_bucket_policy.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_bucket_policy::delete_bucket_policy(directory& dir)
    : m_dir(dir) {}

bool delete_bucket_policy::can_handle(const ep::http::request& req) {
    return req.method() == verb::delete_ && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("policy");
}

coro<response> delete_bucket_policy::handle(request& req) {

    co_await m_dir.set_bucket_policy(req.bucket(), {});
    co_return response(status::no_content);
}

std::string delete_bucket_policy::action_id() const {
    return "s3:DeleteBucketPolicy";
}

} // namespace uh::cluster
