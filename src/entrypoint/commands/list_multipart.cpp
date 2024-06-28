#include "list_multipart.h"
#include "common/utils/strings.h"
#include "entrypoint/http/command_exception.h"

#include <boost/property_tree/ptree.hpp>

namespace uh::cluster {

static http_response
get_response(const std::string& bucket_name,
             const std::map<std::string, std::string>& ongoing) noexcept {

    boost::property_tree::ptree pt;
    boost::property_tree::ptree bucket_node;

    for (const auto& val : ongoing) {
        boost::property_tree::ptree upload_node;
        upload_node.put("Key", val.second);
        upload_node.put("UploadId", val.first);
        bucket_node.add_child("Upload", upload_node);
    }

    pt.add_child("ListMultipartUploadsResult.Bucket", bucket_node);

    http_response res;
    res << pt;
    return res;
}

list_multipart::list_multipart(const reference_collection& collection)
    : m_collection(collection) {}

bool list_multipart::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && req.query("uploads");
}

coro<void> list_multipart::handle(http_request& req) const {
    metric<entrypoint_list_multipart_req>::increase(1);
    const std::string& bucket_name = req.bucket();

    auto ongoing =
        co_await m_collection.uploads.list_multipart_uploads(bucket_name);
    if (ongoing.empty()) {
        throw command_exception(http::status::not_found, "NoMultiPartUploads",
                                "no multipart uploads");
    }

    auto res = get_response(bucket_name, ongoing);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
