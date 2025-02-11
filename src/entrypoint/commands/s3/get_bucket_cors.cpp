#include "get_bucket_cors.h"

#include "entrypoint/http/string_body.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_bucket_cors::get_bucket_cors(directory& dir)
    : m_dir(dir) {}

bool get_bucket_cors::can_handle(const ep::http::request& req) {
    return req.method() == verb::get && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("cors");
}

coro<response> get_bucket_cors::handle(request& req) {

    auto cors = co_await m_dir.get_bucket_cors(req.bucket());

    if (cors) {
        response r;
        r.set_body(std::make_unique<string_body>(std::move(*cors)));
        co_return r;
    } else {
        co_return response(status::no_content);
    }
}

std::string get_bucket_cors::action_id() const { return "s3:GetBucketCORS"; }

} // namespace uh::cluster
