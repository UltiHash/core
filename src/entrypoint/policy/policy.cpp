#include "policy.h"

namespace uh::cluster::ep::policy {

policy::policy(std::string id, std::list<matcher> matchers,
               ep::policy::effect effect)
    : m_id(std::move(id)),
      m_matchers(std::move(matchers)),
      m_effect(effect) {}

std::optional<ep::policy::effect> policy::check(const variables& vars) const {
    for (const auto& matcher : m_matchers) {
        if (!matcher(vars)) {
            return {};
        }
    }

    return m_effect;
}

} // namespace uh::cluster::ep::policy
