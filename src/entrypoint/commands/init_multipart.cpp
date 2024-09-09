#include "init_multipart.h"

#include "entrypoint/http/command_exception.h"

#include <boost/property_tree/ptree.hpp>

namespace uh::cluster {

namespace {

http_response get_response(const http_request& req,
                           const std::string& upload_id) noexcept {
    http_response res;

    boost::property_tree::ptree pt;
    pt.put("InitiateMultipartUploadResult.Bucket", req.bucket());
    pt.put("InitiateMultipartUploadResult.Key", req.object_key());
    pt.put("InitiateMultipartUploadResult.UploadId", upload_id);

    res << pt;

    return res;
}

} // namespace

init_multipart::init_multipart(directory& dir, multipart_state& uploads)
    : m_directory(dir),
      m_uploads(uploads) {}

bool init_multipart::can_handle(const http_request& req) {
    return req.method() == method::post &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploads");
}

coro<http_response> init_multipart::handle(http_request& req) {
    metric<entrypoint_init_multipart_req>::increase(1);
    try {
        co_await m_directory.bucket_exists(req.bucket());
    } catch (const error_exception& e) {
        throw_from_error(e.error());
    }

    const auto upload_id = co_await m_uploads.insert_upload(
        req.bucket(), req.object_key(), req.header("Content-Type"));

    co_return get_response(req, upload_id);
}

std::string init_multipart::action_id() const {
    return "s3:CreateMultipartUpload";
}

} // namespace uh::cluster
