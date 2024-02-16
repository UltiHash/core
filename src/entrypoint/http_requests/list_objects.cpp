#include "list_objects.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

list_objects::list_objects(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool list_objects::can_handle(const http_request& req) {
    const auto& uri = req.get_URI();

    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && !uri.get_query_parameters().empty();
}

static http_response
get_response(const std::vector<std::string>& buckets_found) noexcept {
    http_response res;

    return res;
}

coro<http_response> list_objects::handle(const http_request& req) const {}

} // namespace uh::cluster
