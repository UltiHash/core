#pragma once

#include "effect.h"
#include "entrypoint/commands/command.h"
#include "entrypoint/directory.h"
#include "entrypoint/http/request.h"
#include "policy.h"
#include <filesystem>

namespace uh::cluster::ep::policy {

class module {
public:
    module(directory& dir);
    module(std::list<policy> policies);

    /**
     * Check configured policies to determine whether the provided
     * request is allowed to proceed.
     */
    coro<effect> check(const http::request& request, const command& cmd) const;

    static const std::filesystem::path GLOBAL_CONFIG;

private:
    directory& m_directory;
    std::list<policy> m_policies;
};

} // namespace uh::cluster::ep::policy
