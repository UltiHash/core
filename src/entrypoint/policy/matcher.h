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

#include "variables.h"
#include <functional>
#include <list>

namespace uh::cluster::ep::policy {

enum class undefined_variable { ignore, do_not_match };

typedef std::function<bool(const variables& vars)> matcher;

inline matcher match_always() {
    return [](const variables&) { return true; };
}

inline matcher match_never() {
    return [](const variables&) { return false; };
}

/*
 * Implements logical AND for multiple condition operators
 * See
 * https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_condition-logic-multiple-context-keys-or-values.html
 */
inline matcher conjunction(std::list<matcher> subs) {
    return [subs = std::move(subs)](const variables& vars) {
        for (const auto& m : subs) {
            if (!m(vars)) {
                return false;
            }
        }

        return true;
    };
}

/*
 * Implements logical OR for multiple values for a context key
 */
bool match_any(const auto& list, auto pred) {
    for (const auto& opt : list) {
        if (pred(opt)) {
            return true;
        }
    }

    return false;
}

} // namespace uh::cluster::ep::policy
