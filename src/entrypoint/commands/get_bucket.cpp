#include "get_bucket.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

get_bucket::get_bucket(const reference_collection& collection)
    : m_collection(collection) {}

bool get_bucket::can_handle(const http_request& req) {
    return req.method() == method::get && !req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
}

static http_response get_response(const std::string& bucket_name) noexcept {
    http_response res;

    std::string bucket_xml = "<Bucket>" + bucket_name + "</Bucket>\n";
    res.set_body(
        std::string("<GetBucketResult>\n" + bucket_xml + "</GetBucketResult>"));

    return res;
}

coro<void> get_bucket::handle(http_request& req) const {
    metric<entrypoint_get_bucket_req>::increase(1);
    auto bucket_name = req.bucket();

    try {
        auto client = m_collection.directory_services.get();
        co_await client->bucket_exists(bucket_name);

        auto res = get_response(bucket_name);
        co_await req.respond(res.get_prepared_response());

    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to get bucket `" << bucket_name << "`: " << e;
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
