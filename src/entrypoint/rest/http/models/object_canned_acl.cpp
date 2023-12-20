#include "object_canned_acl.h"
#include "entrypoint/rest/utils/hashing/hash.h"

namespace uh::cluster::rest::http::model
{

    namespace hash_utils = rest::utils::hashing;

    static const int private__HASH = hash_utils::hash_string("private");
    static const int public_read_HASH = hash_utils::hash_string("public-read");
    static const int public_read_write_HASH = hash_utils::hash_string("public-read-write");
    static const int authenticated_read_HASH = hash_utils::hash_string("authenticated-read");
    static const int aws_exec_read_HASH = hash_utils::hash_string("aws-exec-read");
    static const int bucket_owner_read_HASH = hash_utils::hash_string("bucket-owner-read");
    static const int bucket_owner_full_control_HASH = hash_utils::hash_string("bucket-owner-full-control");


    object_canned_acl get_object_canned_acl_for_name(const std::string& name)
    {

        int hash_code = hash_utils::hash_string(name.c_str());

        if (hash_code == private__HASH)
        {
            return object_canned_acl::private_;
        }
        else if (hash_code == public_read_HASH)
        {
            return object_canned_acl::public_read;
        }
        else if (hash_code == public_read_write_HASH)
        {
            return object_canned_acl::public_read_write;
        }
        else if (hash_code == authenticated_read_HASH)
        {
            return object_canned_acl::authenticated_read;
        }
        else if (hash_code == aws_exec_read_HASH)
        {
            return object_canned_acl::aws_exec_read;
        }
        else if (hash_code == bucket_owner_read_HASH)
        {
            return object_canned_acl::bucket_owner_read;
        }
        else if (hash_code == bucket_owner_full_control_HASH)
        {
            return object_canned_acl::bucket_owner_full_control;
        }

        return object_canned_acl::NOT_SET;

    }

    std::string get_name_for_object_canned_acl(object_canned_acl value)
    {

        switch(value)
        {
            case object_canned_acl::private_:
                return "private";
            case object_canned_acl::public_read:
                return "public-read";
            case object_canned_acl::public_read_write:
                return "public-read-write";
            case object_canned_acl::authenticated_read:
                return "authenticated-read";
            case object_canned_acl::aws_exec_read:
                return "aws-exec-read";
            case object_canned_acl::bucket_owner_read:
                return "bucket-owner-read";
            case object_canned_acl::bucket_owner_full_control:
                return "bucket-owner-full-control";
            default:
                return {};
        }

    }

} // uh::cluster::rest::http::model