#include "copy_object.h"
#include "entrypoint/formats.h"
#include "entrypoint/utils.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/url/url.hpp>

namespace uh::cluster {

copy_object::copy_object(directory& dir, global_data_view& gdv)
    : m_directory(dir),
      m_gdv(gdv) {}

bool copy_object::can_handle(const http_request& req) {
    return req.method() == method::put &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.header("x-amz-copy-source");
}

coro<http_response> copy_object::handle(http_request& req) {
    auto copy_source = req.header("x-amz-copy-source");
    if (!copy_source) {
        throw std::runtime_error("x-amz-copy-source not defined");
    }

    boost::urls::url url;
    url.set_encoded_path(*copy_source);

    auto [src_bucket, src_key] = extract_bucket_and_object(url);
    auto src_obj = co_await m_directory.get_object(src_bucket, src_key);

    if (auto ifmatch = req.header("x-amz-copy-source-if-match"); ifmatch) {
        if (src_obj.etag == *ifmatch) {
            co_await m_gdv.link(req.context(), src_obj.addr.value());
            co_await m_directory.copy_object(src_bucket, src_key, req.bucket(),
                                             req.object_key());
        }
    } else {
        co_await m_gdv.link(req.context(), src_obj.addr.value());
        co_await m_directory.copy_object(src_bucket, src_key, req.bucket(),
                                         req.object_key());
    }

    auto obj = co_await m_directory.head_object(req.bucket(), req.object_key());

    boost::property_tree::ptree pt;
    pt.put("CopyObjectResult.LastModified", iso8601_date(obj.last_modified));
    if (obj.etag) {
        pt.put("CopyObjectResult.ETag", *obj.etag);
    }

    http_response res;
    res << pt;

    co_return res;
}

} // namespace uh::cluster
