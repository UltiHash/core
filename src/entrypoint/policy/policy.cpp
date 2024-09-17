#include "policy.h"

namespace uh::cluster::ep::policy {

policy::policy(std::string id, std::list<matcher> matchers,
               ep::policy::effect effect)
    : m_id(std::move(id)),
      m_matchers(std::move(matchers)),
      m_effect(effect) {}

std::optional<ep::policy::effect> policy::check(const http::request& req,
                                                const command& cmd) const {
    for (const auto& matcher : m_matchers) {
        if (!matcher(req, cmd)) {
            return {};
        }
    }

    return m_effect;
}

} // namespace uh::cluster::ep::policy
