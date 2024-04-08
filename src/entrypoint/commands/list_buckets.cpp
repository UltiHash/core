#include "list_buckets.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

list_buckets::list_buckets(const reference_collection& collection)
    : m_collection(collection) {}

bool list_buckets::can_handle(const http_request& req) {

    const auto& uri = req.get_uri();

    return req.get_method() == method::get && uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

static http_response
get_response(const std::vector<std::string>& buckets_found) noexcept {
    http_response res;

    std::string buckets_xml;
    for (const auto& bucket : buckets_found) {
        buckets_xml += "<Bucket>\n"
                       "<Name>" +
                       bucket +
                       "</Name>\n"
                       "</Bucket>\n";
    }

    res.set_body(std::string("<ListAllMyBucketsResult>\n"
                             "   <Buckets>\n" +
                             buckets_xml + "</Buckets>\n" +
                             "</ListAllMyBucketsResult>"));

    return res;
}

coro<void> list_buckets::handle(http_request& req) const {
    metric<entrypoint_list_buckets_req>::increase(1);

    auto client = m_collection.directory_services.get();
    auto mgr = co_await client->acquire_messenger();

    co_await mgr->send(DIRECTORY_BUCKET_LIST_REQ, {});

    const auto h = co_await mgr->recv_header();
    auto result = co_await mgr->recv_directory_list_buckets_message(h);

    std::vector<std::string> buckets_found;
    for (auto& bucket : result.entities) {
        buckets_found.emplace_back(std::move(bucket));
    }

    auto res = get_response(buckets_found);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
