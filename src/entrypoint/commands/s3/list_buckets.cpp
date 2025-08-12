#include "list_buckets.h"

#include <boost/property_tree/ptree.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

response get_response(const std::vector<std::string>& buckets_found) noexcept {

    boost::property_tree::ptree pt;
    boost::property_tree::ptree buckets_node;

    for (const auto& bucket : buckets_found) {
        boost::property_tree::ptree bucket_node;
        bucket_node.put("Name", bucket);
        buckets_node.add_child("Bucket", bucket_node);
    }

    pt.add_child("ListAllMyBucketsResult.Buckets", buckets_node);

    response res;
    res << pt;

    return res;
}

} // namespace

list_buckets::list_buckets(directory& dir)
    : m_directory(dir) {}

bool list_buckets::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket().empty() &&
           req.object_key().empty() && !req.query("versions");
}

coro<response> list_buckets::handle(request& req) {
    metric<entrypoint_list_buckets_req>::increase(1);
    auto buckets = co_await m_directory.list_buckets();
    co_return get_response(buckets);
}

std::string list_buckets::action_id() const { return "s3:ListBuckets"; }

} // namespace uh::cluster
