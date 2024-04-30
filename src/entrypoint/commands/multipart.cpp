#include "multipart.h"
#include "common/utils/md5.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

multipart::multipart(reference_collection& collection)
    : m_collection(collection) {}

bool multipart::can_handle(const http_request& req) {
    return req.method() == method::put && !req.bucket().empty() &&
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

coro<void> multipart::handle(http_request& req) const {
    metric<entrypoint_multipart_req>::increase(1);

    validate(req);
    std::vector<char> buffer(req.content_length());

    auto size = co_await req.read_body(buffer);
    buffer.resize(size);

    dedupe_response resp = {};
    if (!buffer.empty()) {
        resp = co_await m_collection.dedupe_services.get()->deduplicate(
            {buffer.data(), buffer.size()});
    }

    m_collection.server_state.m_uploads.append_upload_part_info(
        *req.query("uploadId"), std::stoi(*req.query("partNumber")), resp,
        buffer);

    http_response res;
    res.set_etag(calculate_md5(buffer));

    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
