#include "delete_object.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_object::delete_object(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_object::can_handle(const http_request& req) {
    return req.method() == method::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("uploadId");
}

coro<http_response> delete_object::handle(http_request& req) const {
    metric<entrypoint_delete_object_req>::increase(1);
    try {
        auto object = co_await m_collection.directory.head_object(
            req.bucket(), req.object_key());

        co_await m_collection.directory.delete_object(req.bucket(),
                                                      req.object_key());

        m_collection.limits.free_storage_size(object.size);
    } catch (const error_exception& e) {
        throw_from_error(e.error());
    }

    co_return http_response{};
}

} // namespace uh::cluster
