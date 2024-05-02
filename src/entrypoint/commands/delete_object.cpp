#include "delete_object.h"
#include "common/coroutines/coro_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_object::delete_object(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_object::can_handle(const http_request& req) {
    return req.method() == method::delete_ && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("uploadId");
}

coro<void> delete_object::handle(http_request& req) const {
    metric<entrypoint_delete_object_req>::increase(1);
    try {
        auto func = [&req](std::shared_ptr<directory_interface> dir,
                           size_t id) -> coro<void> {
            co_await dir->delete_object(req.bucket(), req.object_key());
        };

        co_await broadcast<directory_interface>(
            m_collection.ioc, func,
            m_collection.directory_services.get_services());

        http_response res;

        LOG_DEBUG() << "delete_object response: " << res;
        co_await req.respond(res.get_prepared_response());
    } catch (const error_exception& e) {
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
