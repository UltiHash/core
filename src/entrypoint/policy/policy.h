#ifndef CORE_ENTRYPOINT_POLICY_POLICY_H
#define CORE_ENTRYPOINT_POLICY_POLICY_H

#include "action.h"
#include "matcher.h"
#include <list>

namespace uh::cluster::ep::policy {

class policy {
public:
    policy(std::list<matcher> matchers, action action);

    std::optional<action> check(const http_request& req,
                                const command& cmd) const;

private:
    std::list<matcher> m_matchers;
    action m_action;
};

} // namespace uh::cluster::ep::policy

#endif
