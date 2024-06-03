#include "abort_multipart.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_response.h"

namespace uh::cluster {

abort_multipart::abort_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool abort_multipart::can_handle(const http_request& req) {
    return req.method() == method::delete_ && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploadId");
}

coro<void> abort_multipart::handle(http_request& req) const {
    metric<entrypoint_abort_multipart_req>::increase(1);

    m_collection.uploads.remove_upload(*req.query("uploadId"));

    http_response resp;
    co_await req.respond(resp.get_prepared_response());
}

} // namespace uh::cluster
