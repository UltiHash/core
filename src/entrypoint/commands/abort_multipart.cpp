#include "abort_multipart.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

abort_multipart::abort_multipart(multipart_state& uploads)
    : m_uploads(uploads) {}

bool abort_multipart::can_handle(const request& req) {
    return req.method() == verb::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploadId");
}

coro<response> abort_multipart::handle(request& req) {
    metric<entrypoint_abort_multipart_req>::increase(1);

    auto upload_id = *req.query("uploadId");

    try {
        co_await m_uploads.remove_upload(upload_id);
    } catch (const std::exception& e) {
        throw command_exception(
            status::not_found, "NoSuchUpload",
            "The specified multipart upload does not exist. The upload ID "
            "might not be valid, or the multipart upload might have been "
            "aborted or completed.");
    }

    co_return response{};
}

std::string abort_multipart::action_id() const {
    return "s3:AbortMultipartUpload";
}

} // namespace uh::cluster
