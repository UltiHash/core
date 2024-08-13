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

static void validate(const http_request& req) {
    auto part_num = std::stoi(*req.query("partNumber"));
    if (part_num < 1 || part_num > 10000) {
        throw command_exception(http::status::bad_request, "BadPartNumber",
                                "part number is invalid");
    }
}

coro<http_response> multipart::handle(http_request& req) const {
    metric<entrypoint_multipart_req>::increase(1);

    validate(req);
    unique_buffer<char> buffer(req.content_length());

    if (auto expect = req.header("expect");
        expect && *expect == "100-continue") {
        LOG_INFO() << req.socket().remote_endpoint()
                   << ": sending 100 CONTINUE";
        co_await write(req.socket(), http_response(http::status::continue_));
    }

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
        *req.query("uploadId"), std::stoi(*req.query("partNumber")), resp,
        buffer.size(), std::move(md5));

    co_return res;
}

} // namespace uh::cluster
