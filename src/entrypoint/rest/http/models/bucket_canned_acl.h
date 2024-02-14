#pragma once

#include <string>

namespace uh::cluster::rest::http::model {

enum class bucket_canned_acl {
    NOT_SET,
    private_,
    public_read,
    public_read_write,
    authenticated_read
};

bucket_canned_acl get_bucket_canned_ACL_for_name(const std::string& name);
std::string get_name_for_bucket_canned_ACL(bucket_canned_acl value);

} // namespace uh::cluster::rest::http::model