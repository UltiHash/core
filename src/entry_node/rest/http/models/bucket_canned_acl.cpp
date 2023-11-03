#include "bucket_canned_acl.h"
#include "entry_node/rest/utils/hashing/hash.h"

namespace uh::cluster::rest::http::model
{

    namespace hash_utils = rest::utils::hashing;

    static const int private__HASH = hash_utils::hash_string("private");
    static const int public_read_HASH = hash_utils::hash_string("public-read");
    static const int public_read_write_HASH = hash_utils::hash_string("public-read-write");
    static const int authenticated_read_HASH = hash_utils::hash_string("authenticated-read");

    bucket_canned_acl get_bucket_canned_ACL_for_name(const std::string& name)
    {
        int hashCode = hash_utils::hash_string(name.c_str());
        if (hashCode == private__HASH)
        {
            return bucket_canned_acl::private_;
        }
        else if (hashCode == public_read_HASH)
        {
            return bucket_canned_acl::public_read;
        }
        else if (hashCode == public_read_write_HASH)
        {
            return bucket_canned_acl::public_read_write;
        }
        else if (hashCode == authenticated_read_HASH)
        {
            return bucket_canned_acl::authenticated_read;
        }

        return bucket_canned_acl::NOT_SET;
    }

    std::string get_name_for_bucket_canned_ACL(bucket_canned_acl enumValue)
    {
        switch(enumValue)
        {
            case bucket_canned_acl::private_:
                return "private";
            case bucket_canned_acl::public_read:
                return "public-read";
            case bucket_canned_acl::public_read_write:
                return "public-read-write";
            case bucket_canned_acl::authenticated_read:
                return "authenticated-read";
            default:
                return {};
        }
    }

} // uh::cluster::rest::http::model
