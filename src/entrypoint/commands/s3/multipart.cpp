#include "multipart.h"
#include "common/crypto/hash.h"
#include "common/telemetry/metrics.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

multipart::multipart(deduplicator_interface& dedupe, global_data_view& gdv,
                     multipart_state& uploads)
    : m_dedupe(dedupe),
      m_gdv(gdv),
      m_uploads(uploads) {}

bool multipart::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.query("partNumber") && req.query("uploadId");
}

coro<void> multipart::validate(const request& req) {
    std::size_t part_num = *query<std::size_t>(req, "partNumber");

    if (part_num < 1 || part_num > 10000) {
        throw command_exception(status::bad_request, "InvalidPart",
                                "Part number is invalid.");
    }

    co_return;
}

coro<response> multipart::handle(request& req) {
    metric<entrypoint_multipart_req>::increase(1);

    unique_buffer<char> buffer(req.content_length());
    {
        auto load_body_ctx = req.context().sub_context("load_body");
        auto size = co_await req.read_body(buffer.span());
        buffer.resize(size);
    }

    dedupe_response resp = {};
    {
        auto dedupe_ctx = req.context().sub_context("call_dedupe");
        if (!buffer.empty()) {
            resp = co_await m_dedupe.deduplicate(
                req.context(), {buffer.data(), buffer.size()});
        }
    }

    auto md5 = to_hex(md5::from_buffer(buffer.span()));

    std::string id = *query(req, "uploadId");
    std::size_t part_id = *query<std::size_t>(req, "partNumber");
    response res;
    res.set("ETag", md5);

    req.context().set_attribute("multipart-uploadId", id);
    req.context().set_attribute("multipart-part-number", part_id);

    std::optional<upload_info::part> existing_part;

    {
        auto instance = co_await m_uploads.get();
        auto lock = co_await instance.lock_upload(id);

        try {
            existing_part = co_await instance.part_details(id, part_id);
        } catch (const command_exception&) {
        }

        co_await instance.append_upload_part_info(
            id, part_id, resp, resp.addr.data_size(), std::move(md5));
    }

    if (existing_part) {
        co_await m_gdv.unlink(req.context(), existing_part->addr);
    }

    co_return res;
}

std::string multipart::action_id() const { return "s3:UploadPart"; }

} // namespace uh::cluster
