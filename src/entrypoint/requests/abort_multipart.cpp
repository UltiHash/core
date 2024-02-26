#include "abort_multipart.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

abort_multipart::abort_multipart(entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool abort_multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::delete_ &&
           !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
           uri.query_string_exists("uploadId");
}

static void validate(const http_request& req) {
    const auto& uri = req.get_uri();

    if (uri.get_query_parameters().at("uploadId").empty()) {
        throw rest::http::model::custom_error_response_exception(
            boost::beast::http::status::bad_request,
            rest::http::model::error::type::bad_upload_id);
    }
}

coro<http_response> abort_multipart::handle(const http_request& req) const {
    metric<entrypoint_abort_multipart>::increase(1);
    validate(req);

    const auto& uri = req.get_uri();
    const auto& upload_id = uri.get_query_parameters().at("uploadId");
    const auto& bucket_name = uri.get_bucket_id();
    const auto& object_name = uri.get_object_key();

    if (!m_state.server_state.m_uploads.contains_upload(upload_id)) {
        throw rest::http::model::custom_error_response_exception(
            http::status::not_found,
            rest::http::model::error::type::no_such_upload);
    } else {
        m_state.server_state.m_uploads.remove_upload(upload_id, bucket_name,
                                                     object_name);
    }

    co_return http_response();
}

} // namespace uh::cluster
