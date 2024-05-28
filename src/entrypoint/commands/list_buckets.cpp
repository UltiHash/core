#include "list_buckets.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

list_buckets::list_buckets(const reference_collection& collection)
    : m_collection(collection) {}

bool list_buckets::can_handle(const http_request& req) {
    return req.method() == method::get && req.bucket().empty() &&
           req.object_key().empty() && !req.has_query();
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
    auto buckets = co_await m_collection.directory.list_buckets();
    auto res = get_response(buckets);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
