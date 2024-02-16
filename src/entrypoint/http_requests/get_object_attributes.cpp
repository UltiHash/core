#include "get_object_attributes.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

get_object_attributes::get_object_attributes(
    const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool get_object_attributes::can_handle(const http_request& req) {
    const auto& uri = req.get_URI();

    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() &&
           uri.query_string_exists("attributes");
}

coro<http_response>
get_object_attributes::handle(const http_request& req) const {
    throw rest::http::model::custom_error_response_exception(
        boost::beast::http::status::not_implemented);
}

} // namespace uh::cluster
