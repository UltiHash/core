#ifndef CORE_ENTRYPOINT_USER_USER_H
#define CORE_ENTRYPOINT_USER_USER_H

#include "common/types/common_types.h"
#include "entrypoint/policy/policy.h"
#include <list>
#include <optional>
#include <string>

namespace uh::cluster::ep::user {

struct key {
    std::string id;
    std::string secret_key;
    std::optional<std::string> session_token;
    std::optional<utc_time> expires;
};

struct user {
    std::string id;
    std::string name;
    std::optional<std::string> arn = ANONYMOUS_ARN;

    std::map<std::string, std::string> policy_json;
    std::map<std::string, std::list<policy::policy>> policies;

    std::optional<key> access_key;

    inline static const std::string ANONYMOUS = "anonymous";
    inline static const std::string ANONYMOUS_ARN = "arn:aws:iam::1:anonymous";
};

} // namespace uh::cluster::ep::user

#endif
