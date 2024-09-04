#ifndef CORE_ENTRYPOINT_POLICY_POLICY_H
#define CORE_ENTRYPOINT_POLICY_POLICY_H

#include "action.h"
#include "matcher.h"

#include <list>
#include <optional>

namespace uh::cluster::ep::policy {

class policy {
public:
    policy(std::string id, std::list<matcher> matchers, action action);

    std::optional<action> check(const http_request& req,
                                const command& cmd) const;

    const std::string& id() const { return m_id; }
    action effect() const { return m_action; }

private:
    std::string m_id;
    std::list<matcher> m_matchers;
    action m_action;
};

} // namespace uh::cluster::ep::policy

#endif
