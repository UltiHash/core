#include "get_bucket_versioning.h"

#include <boost/property_tree/ptree.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_bucket_versioning::get_bucket_versioning(directory& dir)
    : m_dir(dir) {}

bool get_bucket_versioning::can_handle(const ep::http::request& req) {
    return req.method() == verb::get && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("versioning") && !req.query("versions");
}

coro<response> get_bucket_versioning::handle(request& req) {

    auto versioning = co_await m_dir.get_bucket_versioning(req.bucket());

    boost::property_tree::ptree result_node;
    put(result_node, "<xmlattr>.xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");
    if (versioning != bucket_versioning::disabled) {
        result_node.put("Status", to_string(versioning));
    }

    boost::property_tree::ptree pt;
    pt.add_child("VersionConfiguration", result_node);

    response r; r << pt;
    co_return r;
}

std::string get_bucket_versioning::action_id() const { return "s3:GetBucketVersioning"; }

} // namespace uh::cluster

