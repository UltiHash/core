#ifndef CORE_ENTRYPOINT_USER_USER_H
#define CORE_ENTRYPOINT_USER_USER_H

#include "entrypoint/policy/policy.h"
#include <list>
#include <string>

namespace uh::cluster::ep::user {

struct user {
    std::string secret_key;
    std::list<policy::policy> policies;

    std::string arn = ANONYMOUS_ARN;

    inline static const std::string ANONYMOUS_ARN = "arn:aws:iam::1:anonymous";
};

} // namespace uh::cluster::ep::user

#endif
