#include "put_object.h"

#include "common/utils/double_buffer.h"
#include <entrypoint/constant.h>

using namespace boost;
using namespace uh::cluster::ep::http;

namespace uh::cluster {

put_object::put_object(limits& uhlimits,
                       directory& dir, storage::global::global_data_view& gdv,
                       deduplicator_interface& dedup)
    : m_dir(dir),
      m_gdv(gdv),
      m_limits(uhlimits),
      m_dedup(dedup) {}

bool put_object::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           !req.query("uploadId") && !req.header("x-amz-copy-source");
}

coro<void> put_object::validate(const request& req) {
    co_await m_dir.bucket_exists(req.bucket());
}

coro<response> put_object::handle(request& req) {

    metric<entrypoint_put_object_req>::increase(1);
    response res;

    auto content_length = req.content_length();
    m_limits.check_storage_size(content_length);

    md5 hash;

    dedupe_response resp = co_await dedupe(req, hash);

    auto tag = to_hex(hash.finalize());
    LOG_DEBUG() << req.peer() << ": etag: " << tag;

    auto original_size = resp.addr.data_size();
    object obj{.name = req.object_key(),
                .size = original_size,
                .addr = std::move(resp.addr),
                .etag = tag,
                .mime = req.header("Content-Type")
                            .value_or(ep::DEFAULT_OBJECT_CONTENT_TYPE)};

    auto version = co_await safe_put_object(m_dir, m_gdv, req.bucket(), obj);

    metric<entrypoint_ingested_data_counter, mebibyte, double>::increase(
        static_cast<double>(content_length) / MEBI_BYTE);

    res.set("ETag", tag);
    res.set("X-Amz-Version-Id", version);
    res.set_original_size(original_size);
    res.set_effective_size(resp.effective_size);

    co_return res;
}

coro<dedupe_response> put_object::dedupe(request& req, md5& hash) const {
    LOG_DEBUG() << req.peer() << ": dedupe putobject";
    auto& b = req.body();
    auto bs = b.buffer_size();

    dedupe_response rv;

    co_await b.consume();
    std::span<const char> data = co_await b.read(bs);

    hash.consume(data);

    // TODO interleaved transfer with streams:
    // First idea was to only `read()` half the buffer size, then `upload()` that part
    // while `read()`ing the second half. In that case, we cannot call `consume()`
    // after the first `read()`, as the buffer is still needed by `upload()`. We
    // can also not call `consume()` after the second `read` for the same reason. We need
    // to wait until the second `upload()` is finished to release the buffer.
    while (!data.empty()) {
        LOG_DEBUG() << req.peer() << ": (i) read " << data.size() << " bytes from body";
        rv.append(co_await m_dedup.deduplicate(std::string_view{data.data(), data.size()}));
        co_await b.consume();
        data = co_await b.read(bs);
        hash.consume(data);
        LOG_DEBUG() << req.peer() << ": (ii) read " << data.size() << " bytes from body";
    }

    co_return rv;
}

std::string put_object::action_id() const { return "s3:PutObject"; }

} // namespace uh::cluster
