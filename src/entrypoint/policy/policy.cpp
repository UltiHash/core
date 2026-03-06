// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
