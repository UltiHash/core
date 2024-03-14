#include "abort_multipart.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

abort_multipart::abort_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool abort_multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::delete_ &&
           !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
           uri.query_string_exists("uploadId");
}

static void validate(const http_request& req) {
    const auto& uri = req.get_uri();

    if (uri.get_query_parameters().at("uploadId").empty()) {
        throw command_exception(http::status::bad_request,
                                command_error::type::bad_upload_id);
    }
}

coro<void> abort_multipart::handle(http_request& req) const {
    metric<entrypoint_abort_multipart_req>::increase(1);
    validate(req);

    const auto& uri = req.get_uri();
    const auto& upload_id = uri.get_query_parameters().at("uploadId");

    if (!m_collection.server_state.m_uploads.contains_upload(upload_id)) {
        throw command_exception(http::status::not_found,
                                command_error::type::no_such_upload);
    }

    m_collection.server_state.m_uploads.remove_upload(upload_id);

    http_response resp;
    co_await req.respond(resp.get_prepared_response());
}

} // namespace uh::cluster
