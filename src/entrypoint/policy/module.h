/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
