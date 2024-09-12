#ifndef CORE_ENTRYPOINT_POLICY_MODULE_H
#define CORE_ENTRYPOINT_POLICY_MODULE_H

#include "effect.h"
#include "entrypoint/commands/command.h"
#include "entrypoint/http/request.h"
#include "policy.h"

namespace uh::cluster::ep::policy {

class module {
public:
    /**
     * Check configured policies to determine whether the provided
     * request is allowed to proceed.
     */
    effect check(const http::request& request, const command& cmd) const;

private:
    std::list<policy> m_policies;
};

} // namespace uh::cluster::ep::policy

#endif
