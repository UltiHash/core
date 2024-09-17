#include "multipart.h"
#include "common/crypto/hash.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

multipart::multipart(
    roundrobin_load_balancer<deduplicator_interface>& dedupe_services,
    multipart_state& uploads)
    : m_dedupe_services(dedupe_services),
      m_uploads(uploads) {}

bool multipart::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.query("partNumber") && req.query("uploadId");
}

coro<void> multipart::validate(const request& req) {
    std::size_t part_num = *query<std::size_t>(req, "partNumber");

    if (part_num < 1 || part_num > 10000) {
        throw command_exception(status::bad_request, "BadPartNumber",
                                "part number is invalid");
    }

    co_return;
}

coro<response> multipart::handle(request& req) {
    metric<entrypoint_multipart_req>::increase(1);

    unique_buffer<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer.span());
    buffer.resize(size);

    dedupe_response resp = {};
    if (!buffer.empty()) {
        resp = co_await m_dedupe_services.get()->deduplicate(
            req.context(), {buffer.data(), buffer.size()});
    }

    auto md5 = to_hex(md5::from_buffer(buffer.span()));

    response res;
    res.set("ETag", md5);

    co_await m_uploads.append_upload_part_info(
        *query(req, "uploadId"), *query<std::size_t>(req, "partNumber"), resp,
        buffer.size(), std::move(md5));

    co_return res;
}

std::string multipart::action_id() const { return "s3:UploadPart"; }

} // namespace uh::cluster
