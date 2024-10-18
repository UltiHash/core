#include "list_multipart.h"
#include "common/telemetry/metrics.h"
#include "entrypoint/http/command_exception.h"

#include <boost/property_tree/ptree.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

response
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

    response res;
    res << pt;
    return res;
}

} // namespace

list_multipart::list_multipart(multipart_state& uploads)
    : m_uploads(uploads) {}

bool list_multipart::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && req.object_key().empty() &&
           req.query("uploads");
}

coro<response> list_multipart::handle(request& req) {
    metric<entrypoint_list_multipart_req>::increase(1);
    const std::string& bucket_name = req.bucket();

    auto ongoing = co_await m_uploads.list_multipart_uploads(bucket_name);
    if (ongoing.empty()) {
        throw command_exception(status::not_found, "NoMultiPartUploads",
                                "no multipart uploads");
    }

    co_return get_response(bucket_name, ongoing);
}

std::string list_multipart::action_id() const {
    return "s3:ListMultipartUploads";
}

} // namespace uh::cluster
