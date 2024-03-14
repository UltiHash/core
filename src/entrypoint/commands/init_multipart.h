
#ifndef UH_CLUSTER_INIT_MULTIPART_H
#define UH_CLUSTER_INIT_MULTIPART_H

#include "common/utils/worker_pool.h"
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
        const auto& uri = req.get_uri();

        return req.get_method() == method::post &&
               !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
               uri.query_string_exists("uploads");
    }

    [[nodiscard]] coro<void> handle(http_request& req) {
        metric<entrypoint_init_multipart_req>::increase(1);
        try {
            co_await m_collection.workers.
                    io_thread_acquire_messenger_and_post_in_io_threads(
                    m_collection.directory_services.get(),
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
                throw command_exception(boost::beast::http::status::not_found,
                                        command_error::bucket_not_found);
            default:
                throw command_exception(http::status::internal_server_error);
            }
        }

        const auto upload_id =
            m_collection.server_state.m_uploads.insert_upload(
                req.get_uri().get_bucket_id(), req.get_uri().get_object_key());

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

    reference_collection& m_collection;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_INIT_MULTIPART_H
