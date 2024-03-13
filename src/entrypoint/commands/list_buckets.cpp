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
    std::vector<std::string> buckets_found;

    auto func = [](std::vector<std::string>& buckets_found,
                   client::acquired_messenger m) -> coro<void> {
        co_await m.get().send(DIRECTORY_BUCKET_LIST_REQ, {});
        const auto h = co_await m.get().recv_header();
        auto list_buckets_res =
            co_await m.get().recv_directory_list_entities_message(h);
        for (auto& bucket : list_buckets_res.entities) {
            buckets_found.emplace_back(std::move(bucket));
        }
    };

    co_await m_collection.workers.io_thread_acquire_messenger_and_post_in_io_threads(
        m_collection.directory_services.get(),
        std::bind_front(func, std::ref(buckets_found)));

    auto res = get_response(buckets_found);
    co_await req.respond(res.get_prepared_response());

}

} // namespace uh::cluster
