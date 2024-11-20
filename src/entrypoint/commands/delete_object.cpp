#include "delete_object.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_object::delete_object(directory& dir, global_data_view& gdv,
                             limits& uhlimits)
    : m_directory(dir),
      m_gdv(gdv),
      m_limits(uhlimits) {}

bool delete_object::can_handle(const request& req) {
    return req.method() == verb::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("uploadId");
}

coro<response> delete_object::handle(request& req) {
    metric<entrypoint_delete_object_req>::increase(1);

    std::optional<object> obj;

    {
        auto dir = co_await m_directory.get();
        auto lock = dir.lock_object(req.bucket(), req.object_key());

        obj = co_await dir.get_object(req.bucket(), req.object_key());
        co_await dir.delete_object(req.bucket(), req.object_key());
    }

    if (obj && obj->addr) {
        try {
            co_await m_gdv.unlink(req.context(), *obj->addr);
        } catch (const error_exception& e) {
            LOG_WARN() << req.peer()
                       << ": freeing memory on storage failed: " << e.what();
        }
    }

    co_return response{};
}

std::string delete_object::action_id() const { return "s3:DeleteObject"; }

} // namespace uh::cluster
