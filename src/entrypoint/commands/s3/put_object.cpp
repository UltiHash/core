#include "put_object.h"

#include <common/crypto/hash.h>
#include <entrypoint/constant.h>
#include <entrypoint/utils.h>

using namespace boost;
using namespace uh::cluster::ep::http;

namespace uh::cluster {

put_object::put_object(limits& uhlimits,
                       directory& dir, storage::global::global_data_view& gdv,
                       deduplicator_interface& dedupe)
    : m_dir(dir),
      m_gdv(gdv),
      m_limits(uhlimits),
      m_dedupe(dedupe) {}

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
    auto resp = co_await deduplicate(m_dedupe, req.body(), hash);

    auto tag = to_hex(hash.finalize());

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

std::string put_object::action_id() const { return "s3:PutObject"; }

} // namespace uh::cluster
