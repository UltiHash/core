#include "create_bucket.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

create_bucket::create_bucket(directory& dir)
    : m_dir(dir) {}

bool create_bucket::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && req.object_key().empty() &&
           !req.query("policy") && !req.query("cors") &&
           !req.query("versioning");
}

coro<response> create_bucket::handle(request& req) {
    metric<entrypoint_create_bucket_req>::increase(1);
    auto bucket_id = req.bucket();
    co_await m_dir.put_bucket(bucket_id);
    co_return response{};
}

std::string create_bucket::action_id() const { return "s3:CreateBucket"; }

} // namespace uh::cluster
