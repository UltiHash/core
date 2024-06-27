#include "copy_object.h"
#include "entrypoint/formats.h"

#include <boost/property_tree/ptree.hpp>

namespace uh::cluster {

copy_object::copy_object(const reference_collection& collection)
    : m_collection(collection) {}

bool copy_object::can_handle(const http_request& req) {
    return req.method() == method::put &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.header("x-amz-copy-source");
}

coro<void> copy_object::handle(http_request& req) const {
    auto copy_source = req.header("x-amz-copy-source");
    if (!copy_source) {
        throw std::runtime_error("x-amz-copy-source not defined");
    }

    boost::urls::url url;
    url.set_encoded_path(*copy_source);

    auto [src_bucket, src_key] = extract_bucket_and_object(url);

    if (auto ifmatch = req.header("x-amz-copy-source-if-match"); ifmatch) {
        co_await m_collection.directory.copy_object_ifmatch(
            src_bucket, src_key, req.bucket(), req.object_key(), *ifmatch);
    } else {
        co_await m_collection.directory.copy_object(
            src_bucket, src_key, req.bucket(), req.object_key());
    }

    auto obj = co_await m_collection.directory.head_object(req.bucket(),
                                                           req.object_key());

    boost::property_tree::ptree pt;
    pt.put("CopyObjectResult.LastModified", iso8601_date(obj.last_modified));
    if (obj.etag) {
        pt.put("CopyObjectResult.ETag", *obj.etag);
    }

    http_response res;
    res << pt;

    LOG_DEBUG() << req.socket().remote_endpoint()
                << ": copy_object response: " << res;
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
