#include "delete_bucket_cors.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_bucket_cors::delete_bucket_cors(directory& dir)
    : m_dir(dir) {}

bool delete_bucket_cors::can_handle(const ep::http::request& req) {
    return req.method() == verb::delete_ && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("cors");
}

coro<response> delete_bucket_cors::handle(request& req) {

    co_await m_dir.set_bucket_cors(req.bucket(), {});
    co_return response(status::no_content);
}

std::string delete_bucket_cors::action_id() const {
    return "s3:DeleteBucketCORS";
}

} // namespace uh::cluster
