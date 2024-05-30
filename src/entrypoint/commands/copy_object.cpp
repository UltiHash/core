#include "copy_object.h"

#include "entrypoint/formats.h"

namespace uh::cluster {

copy_object::copy_object(const reference_collection& collection)
    : m_collection(collection) {}

bool copy_object::can_handle(const http_request& req) {
    return req.method() == method::put && !req.bucket().empty() &&
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

    http_response res;
    res.set_body(
        "<CopyObjectResult><LastModified>" + iso8601_date(obj.last_modified) +
        "</LastModified>" +
        (obj.etag ? std::string("<ETag>") + *obj.etag + std::string("</ETag>")
                  : std::string()) +
        "</CopyObjectResult>");

    LOG_DEBUG() << req.socket().remote_endpoint()
                << ": copy_object response: " << res;
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
