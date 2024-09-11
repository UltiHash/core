#include "delete_object.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_object::delete_object(directory& dir, global_data_view& gdv,
                             limits& uhlimits)
    : m_directory(dir),
      m_gdv(gdv),
      m_limits(uhlimits) {}

bool delete_object::can_handle(const http_request& req) {
    return req.method() == method::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("uploadId");
}

coro<http_response> delete_object::handle(http_request& req) {
    metric<entrypoint_delete_object_req>::increase(1);
    try {
        object obj;
        if constexpr (m_enable_refcount) {
            obj =
                co_await m_directory.get_object(req.bucket(), req.object_key());
        } else {
            obj = co_await m_directory.head_object(req.bucket(),
                                                   req.object_key());
        }

        co_await m_directory.delete_object(req.bucket(), req.object_key());

        if constexpr (m_enable_refcount) {
            co_await m_gdv.unlink(req.context(), obj.addr.value());
        }

        m_limits.free_storage_size(obj.size);
    } catch (const error_exception& e) {
        throw_from_error(e.error());
    }

    co_return http_response{};
}

std::string delete_object::action_id() const { return "s3:DeleteObject"; }

} // namespace uh::cluster
