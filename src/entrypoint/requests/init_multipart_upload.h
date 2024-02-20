
#ifndef UH_CLUSTER_INIT_MULTIPART_UPLOAD_H
#define UH_CLUSTER_INIT_MULTIPART_UPLOAD_H

#include "common/utils/worker_utils.h"
#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class init_multipart_upload {
  public:
    explicit init_multipart_upload(entrypoint_state& entry_state)
        : m_state(entry_state) {}

    static bool can_handle(const http_request& req) {
        const auto& uri = req.get_uri();

        return req.get_method() == method::post &&
               !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
               uri.query_string_exists("uploads");
    }

    [[nodiscard]] coro<http_response> handle(const http_request& req) {
        try {

            co_await worker_utils::
                io_thread_acquire_messenger_and_post_in_io_threads(
                    m_state.workers, m_state.ioc,
                    m_state.directory_services.get(),
                    [&req](client::acquired_messenger m) -> coro<void> {
                        directory_message dir_req{
                            .bucket_id = req.get_uri().get_bucket_id()};

                        co_await m.get().send_directory_message(
                            DIRECTORY_BUCKET_EXISTS_REQ, dir_req);
                        co_await m.get().recv_header();
                    });

        }

        catch (const error_exception& e) {
            switch (*e.error()) {
            case error::bucket_not_found:
                throw rest::http::model::custom_error_response_exception(
                    boost::beast::http::status::not_found,
                    rest::http::model::error::bucket_not_found);
            default:
                throw rest::http::model::custom_error_response_exception(
                    boost::beast::http::status::internal_server_error);
            }
        }

        const auto upload_id = generate_unique_id();
        if (!m_state.server_state.m_uploads.insert_upload(
                upload_id, req.get_uri().get_bucket_id(),
                req.get_uri().get_object_key())) {
            throw rest::http::model::custom_error_response_exception(
                http::status::internal_server_error);
        }

        co_return get_response(req, upload_id);
    }

  private:
    static http_response get_response(const http_request& req,
                                      const std::string& upload_id) {
        http_response res;

        res.set_body("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<InitiateMultipartUploadResult>\n"
                     "<Bucket>" +
                     req.get_uri().get_bucket_id() +
                     "</Bucket>\n"
                     "<Key>" +
                     req.get_uri().get_object_key() +
                     "</Key>\n"
                     "<UploadId>" +
                     upload_id +
                     "</UploadId>\n"
                     "</InitiateMultipartUploadResult>");

        return res;
    }

    entrypoint_state& m_state;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_INIT_MULTIPART_UPLOAD_H
