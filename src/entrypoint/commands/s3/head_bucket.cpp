#include "head_bucket.h"

#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

head_bucket::head_bucket(directory& dir)
    : m_dir(dir) {}

bool head_bucket::can_handle(const request& req) {
    return req.method() == verb::head && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && req.object_key().empty() &&
           !req.query("attributes");
}

coro<response> head_bucket::handle(request& req) {
    metric<entrypoint_head_object_req>::increase(1);

    co_await m_dir.bucket_exists(req.bucket());

    co_return response{};
}

std::string head_bucket::action_id() const { return "s3:HeadBucket"; }

} // namespace uh::cluster
