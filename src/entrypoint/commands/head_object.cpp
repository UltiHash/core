#include "head_object.h"

#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

head_object::head_object(const reference_collection& coll)
    : m_coll(coll) {}

bool head_object::can_handle(const http_request& req) {
    return req.method() == method::head &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("attributes");
}

coro<http_response> head_object::handle(const http_request& req) const {
    metric<entrypoint_head_object_req>::increase(1);

    try {
        auto obj = co_await m_coll.directory.head_object(req.bucket(),
                                                         req.object_key());

        http_response res;
        res.set("Content-Length", std::to_string(obj.size));
        res.set("Last-Modified", imf_fixdate(obj.last_modified));
        res.set("ETag", obj.etag);
        res.set("Content-Type", obj.mime);

        co_return res;
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }
}

} // namespace uh::cluster
