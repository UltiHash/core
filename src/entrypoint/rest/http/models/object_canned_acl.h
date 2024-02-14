#pragma once

#include <string>

namespace uh::cluster::rest::http::model {

enum class object_canned_acl {
    NOT_SET,
    private_,
    public_read,
    public_read_write,
    authenticated_read,
    aws_exec_read,
    bucket_owner_read,
    bucket_owner_full_control
};

object_canned_acl get_object_canned_acl_for_name(const std::string& name);
std::string get_name_for_object_canned_acl(object_canned_acl value);

} // namespace uh::cluster::rest::http::model