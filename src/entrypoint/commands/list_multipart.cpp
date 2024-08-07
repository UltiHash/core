#include "list_multipart.h"
#include "entrypoint/http/command_exception.h"

#include <boost/property_tree/ptree.hpp>

namespace uh::cluster {

namespace {

http_response
get_response(const std::string& bucket_name,
             const std::map<std::string, std::string>& ongoing) noexcept {

    boost::property_tree::ptree pt;

    for (const auto& val : ongoing) {
        boost::property_tree::ptree upload_node;
        upload_node.put("Key", val.second);
        upload_node.put("UploadId", val.first);
        pt.add_child("ListMultipartUploadsResult.Upload", upload_node);
    }

    pt.put("ListMultipartUploadsResult.Bucket", bucket_name);
    pt.put("ListMultipartUploadsResult.IsTruncated", false);

    http_response res;
    res << pt;
    return res;
}

} // namespace

list_multipart::list_multipart(const reference_collection& collection)
    : m_collection(collection) {}

bool list_multipart::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && req.query("uploads");
}

coro<http_response> list_multipart::handle(http_request& req) const {
    metric<entrypoint_list_multipart_req>::increase(1);
    const std::string& bucket_name = req.bucket();

    auto ongoing =
        co_await m_collection.uploads.list_multipart_uploads(bucket_name);
    if (ongoing.empty()) {
        throw command_exception(http::status::not_found, "NoMultiPartUploads",
                                "no multipart uploads");
    }

    co_return get_response(bucket_name, ongoing);
}

} // namespace uh::cluster
