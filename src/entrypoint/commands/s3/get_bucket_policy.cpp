#include "get_bucket_policy.h"

#include "entrypoint/http/string_body.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_bucket_policy::get_bucket_policy(directory& dir)
    : m_dir(dir) {}

bool get_bucket_policy::can_handle(const ep::http::request& req) {
    return req.method() == verb::get && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("policy");
}

coro<response> get_bucket_policy::handle(request& req) {

    auto policy = co_await m_dir.get_bucket_policy(req.bucket());

    if (policy) {
        response r;
        r.set_body(std::make_unique<string_body>(std::move(*policy)));
        co_return r;
    } else {
        co_return response(status::no_content);
    }
}

std::string get_bucket_policy::action_id() const {
    return "s3:GetBucketPolicy";
}

} // namespace uh::cluster
