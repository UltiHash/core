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

bool match_any(const auto& list, auto pred) {
    for (const auto& opt : list) {
        if (pred(opt)) {
            return true;
        }
    }

    return false;
}

bool match_all(const auto& list, auto pred) { return !match_any(list, pred); }

} // namespace uh::cluster::ep::policy

#endif
