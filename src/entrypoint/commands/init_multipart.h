
#ifndef UH_CLUSTER_INIT_MULTIPART_H
#define UH_CLUSTER_INIT_MULTIPART_H

#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class init_multipart {
public:
    explicit init_multipart(reference_collection& collection)
        : m_collection(collection) {}

    static bool can_handle(const http_request& req) {
        return req.method() == method::post && !req.bucket().empty() &&
               !req.object_key().empty() && req.query("uploads");
    }

    [[nodiscard]] coro<void> handle(http_request& req) {
        metric<entrypoint_init_multipart_req>::increase(1);
        try {
            co_await m_collection.directory.bucket_exists(req.bucket());
        } catch (const error_exception& e) {
            throw_from_error(e.error());
        }

        const auto upload_id =
            m_collection.server_state.m_uploads.insert_upload(req.bucket(),
                                                              req.object_key());

        auto res = get_response(req, upload_id);
        co_await req.respond(res.get_prepared_response());
    }

private:
    static http_response get_response(const http_request& req,
                                      const std::string& upload_id) noexcept {
        http_response res;

        res.set_body("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<InitiateMultipartUploadResult>\n"
                     "<Bucket>" +
                     req.bucket() +
                     "</Bucket>\n"
                     "<Key>" +
                     req.object_key() +
                     "</Key>\n"
                     "<UploadId>" +
                     upload_id +
                     "</UploadId>\n"
                     "</InitiateMultipartUploadResult>");

        return res;
    }

    reference_collection& m_collection;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_INIT_MULTIPART_H
