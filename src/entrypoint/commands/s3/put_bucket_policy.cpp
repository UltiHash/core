#include "put_bucket_policy.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

put_bucket_policy::put_bucket_policy(directory& dir)
    : m_dir(dir) {}

bool put_bucket_policy::can_handle(const ep::http::request& req) {
    return req.method() == verb::put && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("policy");
}

coro<response> put_bucket_policy::handle(request& req) {

    co_await m_dir.set_bucket_policy(req.bucket(), co_await copy_to_buffer(req.body()));
    co_return response(status::no_content);
}

std::string put_bucket_policy::action_id() const {
    return "s3:PutBucketPolicy";
}

} // namespace uh::cluster
