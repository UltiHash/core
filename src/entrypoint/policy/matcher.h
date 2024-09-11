#ifndef CORE_ENTRYPOINT_POLICY_MATCHER_H
#define CORE_ENTRYPOINT_POLICY_MATCHER_H

#include <functional>
#include <list>
#include <map>
#include <set>
#include <string>

namespace uh::cluster {
class command;
class http_request;
} // namespace uh::cluster

namespace uh::cluster::ep::policy {

enum class undefined_variable { ignore, do_not_match };

typedef std::function<bool(const http_request&, const command&)> matcher;

inline matcher match_always() {
    return [](const http_request&, const command&) { return true; };
}

inline matcher match_never() {
    return [](const http_request&, const command&) { return false; };
}

inline matcher conjunction(std::list<matcher> subs) {
    return [subs = std::move(subs)](const http_request& r, const command& c) {
        for (const auto& m : subs) {
            if (!m(r, c)) {
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
