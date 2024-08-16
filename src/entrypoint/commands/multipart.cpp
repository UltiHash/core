#include "multipart.h"
#include "common/utils/md5.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

multipart::multipart(reference_collection& collection)
    : m_collection(collection) {}

bool multipart::can_handle(const http_request& req) {
    return req.method() == method::put &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("partNumber") &&
           req.query("uploadId");
}

coro<void> multipart::validate(const http_request& req) {
    std::size_t part_num = *query<std::size_t>(req, "partNumber");

    if (part_num < 1 || part_num > 10000) {
        throw command_exception(http::status::bad_request, "BadPartNumber",
                                "part number is invalid");
    }

    co_return;
}

coro<http_response> multipart::handle(http_request& req) const {
    metric<entrypoint_multipart_req>::increase(1);

    unique_buffer<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer.span());
    buffer.resize(size);

    dedupe_response resp = {};
    if (!buffer.empty()) {
        resp = co_await m_collection.dedupe_services.get()->deduplicate(
            req.context(), {buffer.data(), buffer.size()});
    }

    auto md5 = calculate_md5(buffer.span());

    http_response res;
    res.set("ETag", md5);

    co_await m_collection.uploads.append_upload_part_info(
        *query(req, "uploadId"), *query<std::size_t>(req, "partNumber"), resp,
        buffer.size(), std::move(md5));

    co_return res;
}

} // namespace uh::cluster
