#include "policy.h"

namespace uh::cluster::ep::policy {

policy::policy(std::string id, std::list<matcher> matchers, action action)
    : m_id(std::move(id)),
      m_matchers(std::move(matchers)),
      m_action(action) {}

std::optional<action> policy::check(const http_request& req,
                                    const command& cmd) const {
    for (const auto& matcher : m_matchers) {
        if (!matcher(req, cmd)) {
            return {};
        }
    }

    return m_action;
}

} // namespace uh::cluster::ep::policy
