#include "delete_object.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_object::delete_object(directory& dir, sn::interface& gdv,
                             limits& uhlimits)
    : m_dir(dir) {}

bool delete_object::can_handle(const request& req) {
    return req.method() == verb::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("uploadId");
}

coro<response> delete_object::handle(request& req) {
    metric<entrypoint_delete_object_req>::increase(1);

    co_await m_dir.bucket_exists(req.bucket());
    co_await m_dir.delete_object(req.bucket(), req.object_key());

    co_return response{};
}

std::string delete_object::action_id() const { return "s3:DeleteObject"; }

} // namespace uh::cluster
