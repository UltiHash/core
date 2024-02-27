#include "get_bucket.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

get_bucket::get_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool get_bucket::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

static http_response get_response(const std::string& bucket_name) noexcept {
    http_response res;

    std::string bucket_xml = "<Bucket>" + bucket_name + "</Bucket>\n";
    res.set_body(
        std::string("<GetBucketResult>\n" + bucket_xml + "</GetBucketResult>"));

    return res;
}

coro<http_response> get_bucket::handle(const http_request& req) const {
    auto bucket_name = req.get_uri().get_bucket_id();

    try {
        auto func = [](const std::string& bucket_name,
                       client::acquired_messenger m) -> coro<void> {
            directory_message dir_req;
            dir_req.bucket_id = bucket_name;
            co_await m.get().send_directory_message(DIRECTORY_BUCKET_EXISTS_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };

        co_await worker_utils::
            io_thread_acquire_messenger_and_post_in_io_threads(
                m_collection.workers, m_collection.ioc,
                m_collection.directory_services.get(),
                std::bind_front(func, std::cref(bucket_name)));

        co_return get_response(bucket_name);

    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to get bucket `" << bucket_name << "`: " << e;

        switch (*e.error()) {
        case error::bucket_not_found:
            throw command_exception(boost::beast::http::status::not_found,
                                    command_error::bucket_not_found);
        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
