#ifndef CORE_ENTRYPOINT_POLICY_MODULE_H
#define CORE_ENTRYPOINT_POLICY_MODULE_H

#include "action.h"
#include "entrypoint/commands/command.h"
#include "entrypoint/http/http_request.h"

namespace uh::cluster::ep::policy {

class module {
public:
    /**
     * Check configured policies to determine whether the provided
     * request is allowed to proceed.
     */
    action check(const http_request& request, const command& cmd);
};

} // namespace uh::cluster::ep::policy

#endif
