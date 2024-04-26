#include "delete_object.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_object::delete_object(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_object::can_handle(const http_request& req) {
    const auto& uri = req.uri();

    return req.method() == method::delete_ && !req.bucket().empty() &&
           !req.object_key().empty() && !uri.has("uploadId");
}

coro<void> delete_object::handle(http_request& req) const {
    metric<entrypoint_delete_object_req>::increase(1);
    try {
        auto cl = m_collection.directory_services.get();
        auto mgr = co_await cl->acquire_messenger();

        directory_message dir_req{
            .bucket_id = req.bucket(),
            .object_key = std::make_unique<std::string>(req.object_key())};
        co_await mgr->send_directory_message(DIRECTORY_OBJECT_DELETE_REQ,
                                             dir_req);
        co_await mgr->recv_header();

        http_response res;

        LOG_DEBUG() << "delete_object response: " << res;
        co_await req.respond(res.get_prepared_response());
    } catch (const error_exception& e) {
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
