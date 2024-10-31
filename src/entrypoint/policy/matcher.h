#ifndef CORE_ENTRYPOINT_POLICY_MATCHER_H
#define CORE_ENTRYPOINT_POLICY_MATCHER_H

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

/*
 * Implements logical NOR for multiple values for a context key
 */
bool match_none(const auto& list, auto pred) {
    for (const auto& opt : list) {
        if (pred(opt)) {
            return false;
        }
    }

    return true;
}

// NOT((NOT A) OR (NOT B)) == A AND B, but we should implement A NOR B
bool match_all(const auto& list, auto pred) { return !match_any(list, pred); }

} // namespace uh::cluster::ep::policy

#endif
