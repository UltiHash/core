#include "delete_object.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

delete_object::delete_object(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool delete_object::can_handle(const http_request& req) {

    if (req.get_method() == method::delete_) {

        if (const auto& uri = req.get_URI();
            !uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
            !uri.query_string_exists("uploadId")) {
            return true;
        }
    }

    return false;
}

coro<http_response> delete_object::handle(const http_request& req) const {}

} // namespace uh::cluster
