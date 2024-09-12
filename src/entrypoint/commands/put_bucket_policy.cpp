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

    constexpr std::size_t buffer_size = 64 * KIBI_BYTE;
    std::string buffer;
    std::size_t pos = 0;
    std::size_t count = 0;

    do {
        buffer.resize(pos + buffer_size);
        count = co_await req.read_body({&buffer[pos], buffer_size});
        pos += count;
        buffer.resize(pos);
    } while (count == buffer_size);

    co_await m_dir.set_bucket_policy(req.bucket(), std::move(buffer));
    co_return response(status::no_content);
}

std::string put_bucket_policy::action_id() const {
    return "s3:PutBucketPolicy";
}

} // namespace uh::cluster
