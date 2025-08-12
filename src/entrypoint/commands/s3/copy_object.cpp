#include "copy_object.h"
#include "entrypoint/formats.h"
#include "entrypoint/utils.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/url/url.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

copy_object::copy_object(directory& dir, storage::global::global_data_view& gdv,
                         limits& limits)
    : m_dir(dir),
      m_gdv(gdv),
      m_limits(limits) {}

bool copy_object::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.header("x-amz-copy-source");
}

coro<response> copy_object::handle(request& req) {
    boost::urls::url url;

    auto source = *req.header("x-amz-copy-source");
    if (!source.starts_with("/")) {
        source = "/" + source;
    }

    url.set_encoded_path(source);

    auto src_bucket = get_bucket_id(url.path());
    auto src_key = get_object_key(url.path());
    std::optional<std::string> version;
    for (const auto& param : url.params()) {
        if (param.key == "versionId") {
            version = param.value;
            break;
        }
    }

    auto obj = co_await m_dir.get_object(src_bucket, src_key, version);

    if (auto ifmatch = req.header("x-amz-copy-source-if-match");
        ifmatch && *ifmatch != obj->etag) {
        throw command_exception(status::precondition_failed,
                                "PreconditionFailed",
                                "At least one of the preconditions that you "
                                "specified did not hold.");
    }

    m_limits.check_storage_size(obj->size);

    auto rejects = co_await m_gdv.link(*obj->addr);
    if (!rejects.empty()) {
        LOG_ERROR() << req.peer()
                    << ": database contains object without references";
        throw command_exception(status::internal_server_error, "InternalError",
                                "Found corrupted data.");
    }

    obj->name = req.object_key();
    auto new_version = co_await safe_put_object(m_dir, m_gdv, req.bucket(), *obj);

    boost::property_tree::ptree pt;
    put(pt, "CopyObjectResult.LastModified", iso8601_date(obj->last_modified));
    put(pt, "CopyObjectResult.ETag", obj->etag);

    response res;
    res << pt;

    res.set("X-Amz-Copy-Source-Version-Id", version);
    res.set("X-Amz-Version-Id", new_version);

    co_return res;
}

std::string copy_object::action_id() const { return "s3:CopyObject"; }

} // namespace uh::cluster
