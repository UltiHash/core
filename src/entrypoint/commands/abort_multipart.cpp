#include "abort_multipart.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_response.h"

namespace uh::cluster {

abort_multipart::abort_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool abort_multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::delete_ &&
           !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
           uri.query_string_exists("uploadId");
}

coro<void> abort_multipart::handle(http_request& req) const {
    metric<entrypoint_abort_multipart_req>::increase(1);

    const auto& uri = req.get_uri();
    const auto& upload_id = uri.get_query_parameters().at("uploadId");

    m_collection.server_state.m_uploads.remove_upload(upload_id);

    http_response resp;
    co_await req.respond(resp.get_prepared_response());
}

} // namespace uh::cluster
