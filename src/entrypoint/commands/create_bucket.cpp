#include "create_bucket.h"
#include "common/coroutines/coro_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

create_bucket::create_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool create_bucket::can_handle(const http_request& req) {
    return req.method() == method::put && !req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
}

coro<void> create_bucket::handle(http_request& req) const {
    metric<entrypoint_create_bucket_req>::increase(1);
    auto bucket_id = req.bucket();
    try {
        auto func = [&bucket_id](std::shared_ptr<directory_interface> dir,
                                 size_t id) -> coro<void> {
            co_await dir->put_bucket(bucket_id);
        };

        co_await broadcast<directory_interface>(
            m_collection.ioc, func,
            m_collection.directory_services.get_services());

        http_response res;
        co_await req.respond(res.get_prepared_response());
    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to add the bucket " << bucket_id
                    << " to the directory: " << e;
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
