#include "get_bucket_versioning.h"

#include <boost/property_tree/ptree.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_bucket_versioning::get_bucket_versioning(directory& dir)
    : m_dir(dir) {}

bool get_bucket_versioning::can_handle(const ep::http::request& req) {
    return req.method() == verb::get && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("versioning");
}

coro<response> get_bucket_versioning::handle(request& req) {

    auto versioning = co_await m_dir.get_bucket_versioning(req.bucket());

    boost::property_tree::ptree pt;
    pt.put("VersioningInformation.Status", to_string(versioning));
    pt.put("VersioningInformation.MFADelete", "Disabled");

    response r; r << pt;
    co_return r;
}

std::string get_bucket_versioning::action_id() const { return "s3:GetBucketVersioning"; }

} // namespace uh::cluster

