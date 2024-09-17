#ifndef CORE_ENTRYPOINT_POLICY_POLICY_H
#define CORE_ENTRYPOINT_POLICY_POLICY_H

#include "effect.h"
#include "matcher.h"

#include <list>
#include <optional>

namespace uh::cluster::ep::policy {

class policy {
public:
    policy(std::string id, std::list<matcher> matchers,
           ep::policy::effect effect);

    std::optional<ep::policy::effect> check(const http::request& req,
                                            const command& cmd) const;

    const std::string& id() const { return m_id; }
    ep::policy::effect effect() const { return m_effect; }

private:
    std::string m_id;
    std::list<matcher> m_matchers;
    ep::policy::effect m_effect;
};

} // namespace uh::cluster::ep::policy

#endif
