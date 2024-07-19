#include "abort_multipart.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

abort_multipart::abort_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool abort_multipart::can_handle(const http_request& req) {
    return req.method() == method::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploadId");
}

coro<http_response> abort_multipart::handle(http_request& req) const {
    metric<entrypoint_abort_multipart_req>::increase(1);

    auto upload_id = *req.query("uploadId");

    try {
        co_await m_collection.uploads.remove_upload(upload_id);
    } catch (const std::exception& e) {
        throw command_exception(
            http::status::not_found, "NoSuchUpload",
            "The specified multipart upload does not exist. The upload ID "
            "might not be valid, or the multipart upload might have been "
            "aborted or completed.");
    }

    co_return http_response{};
}

} // namespace uh::cluster
