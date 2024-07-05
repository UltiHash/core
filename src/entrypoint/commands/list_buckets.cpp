#include "list_buckets.h"
#include "entrypoint/http/command_exception.h"

#include <boost/property_tree/ptree.hpp>

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

    boost::property_tree::ptree pt;
    boost::property_tree::ptree buckets_node;

    for (const auto& bucket : buckets_found) {
        boost::property_tree::ptree bucket_node;
        bucket_node.put("Name", bucket);
        buckets_node.add_child("Bucket", bucket_node);
    }

    pt.add_child("ListAllMyBucketsResult.Buckets", buckets_node);

    res << pt;

    return res;
}

coro<void> list_buckets::handle(http_request& req) const {
    metric<entrypoint_list_buckets_req>::increase(1);
    auto buckets = co_await m_collection.directory.list_buckets();
    auto res = get_response(buckets);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
