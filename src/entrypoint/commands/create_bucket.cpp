#include "create_bucket.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

create_bucket::create_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool create_bucket::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::put && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

http_response create_bucket::handle(http_request& req) const {

    metric<entrypoint_create_bucket_req>::increase(1);

    auto bucket_id = req.get_uri().get_bucket_id();
    auto futures = m_collection.directory_services.broad_cast<void>(
        [bucket_id](auto m) -> coro<void> {
            directory_message dir_req{.bucket_id = bucket_id};
            co_await m.get().send_directory_message(DIRECTORY_BUCKET_PUT_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        });

    try {
        for (auto& f : futures) {
            f.get();
        }
    } catch (const error_exception& e) {
        LOG_ERROR() << "create_bucket: " << e.what();

        if (*e.error() == error::bucket_already_exists) {
            throw command_exception(http::status::conflict,
                                    command_error::bucket_already_exists);
        }

        throw command_exception(http::status::internal_server_error);
    }

    return http_response();
}

} // namespace uh::cluster
