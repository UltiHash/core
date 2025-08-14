#include "multipart.h"
#include "common/crypto/hash.h"
#include "common/telemetry/metrics.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

multipart::multipart(deduplicator_interface& dedupe,
                     storage::global::global_data_view& gdv,
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

    cluster::md5 hash;
    dedupe_response resp = co_await dedupe(req, hash);

    auto md5 = to_hex(hash.finalize());

    std::string id = *query(req, "uploadId");
    std::size_t part_id = *query<std::size_t>(req, "partNumber");
    response res;
    res.set("ETag", md5);

    auto span = co_await boost::asio::this_coro::span;
    span->set_attribute("multipart-uploadId", id);
    span->set_attribute("multipart-part-number", part_id);

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
        co_await m_gdv.unlink(existing_part->addr);
    }

    co_return res;
}

coro<dedupe_response> multipart::dedupe(request& req, md5& hash) const {
    auto& b = req.body();
    auto bs = b.buffer_size();

    co_await b.consume();
    std::span<const char> data = co_await b.read(bs);
    hash.consume(data);

    // TODO interleaved transfer with streams:
    // First idea was to only `read()` half the buffer size, then `upload()` that part
    // while `read()`ing the second half. In that case, we cannot call `consume()`
    // after the first `read()`, as the buffer is still needed by `upload()`. We
    // can also not call `consume()` after the second `read` for the same reason. We need
    // to wait until the second `upload()` is finished to release the buffer.
    dedupe_response rv;
    while (!data.empty()) {
        rv.append(co_await m_dedupe.deduplicate(std::string_view{ data.data(), data.size() }));
        co_await b.consume();
        data = co_await b.read(bs);
    }

    co_return rv;
}

std::string multipart::action_id() const { return "s3:UploadPart"; }

} // namespace uh::cluster
