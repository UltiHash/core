#pragma once

#include "policy.h"
#include <list>
#include <string>

namespace uh::cluster::ep::policy {

class parser {
public:
    static std::list<policy> parse(const std::string& code);

    inline static const std::string IAM_JSON_VERSION = "2012-10-17";
    inline static const std::string UNDEFINED_SID = "<Sid undefined>";
};

} // namespace uh::cluster::ep::policy
