#ifndef CORE_ENTRYPOINT_POLICY_MATCHER_H
#define CORE_ENTRYPOINT_POLICY_MATCHER_H

#include <functional>
#include <list>
#include <map>
#include <set>
#include <string>

namespace uh::cluster {
class http_request;
class command;
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

inline matcher match_all(std::list<matcher> subs) {
    return [subs = std::move(subs)](const http_request& r, const command& c) {
        for (const auto& m : subs) {
            if (!m(r, c)) {
                return false;
            }
        }

        return true;
    };
}

inline matcher match_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const http_request&,
                                          const command& cmd) { return false; };
}

inline matcher match_not_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const http_request&,
                                          const command& cmd) { return false; };
}

inline matcher match_resource(std::set<std::string> resources) {
    return [actions = std::move(resources)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher match_not_resource(std::set<std::string> resources) {
    return [actions = std::move(resources)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher match_principal(std::set<std::string> principals) {
    return [actions = std::move(principals)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher match_not_principal(std::set<std::string> principals) {
    return [actions = std::move(principals)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher
match_stringequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request& r,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_stringnotequals(std::map<std::string, std::list<std::string>> strings,
                      undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher match_stringequalsignorecase(
    std::map<std::string, std::list<std::string>> strings,
    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher match_stringnotequalsignorecase(
    std::map<std::string, std::list<std::string>> strings,
    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_stringlike(std::map<std::string, std::list<std::string>> strings,
                 undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_stringnotlike(std::map<std::string, std::list<std::string>> strings,
                    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_numericequals(std::map<std::string, std::list<std::string>> strings,
                    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_numericnotequals(std::map<std::string, std::list<std::string>> strings,
                       undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_numericlessthan(std::map<std::string, std::list<std::string>> strings,
                      undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher match_numericlessthanequals(
    std::map<std::string, std::list<std::string>> strings,
    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_numericgreaterthan(std::map<std::string, std::list<std::string>> strings,
                         undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher match_numericgreaterthanequals(
    std::map<std::string, std::list<std::string>> strings,
    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_dateequals(std::map<std::string, std::list<std::string>> strings,
                 undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_datenotequals(std::map<std::string, std::list<std::string>> strings,
                    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_datelessthan(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_datelessthanequals(std::map<std::string, std::list<std::string>> strings,
                         undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_dategreaterthan(std::map<std::string, std::list<std::string>> strings,
                      undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher match_dategreaterthanequals(
    std::map<std::string, std::list<std::string>> strings,
    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher match_bool(std::map<std::string, std::list<std::string>> strings,
                          undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_binaryequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_ipaddress(std::map<std::string, std::list<std::string>> strings,
                undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_notipaddress(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_arnequals(std::map<std::string, std::list<std::string>> strings,
                undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_arnlike(std::map<std::string, std::list<std::string>> strings,
              undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_arnnotequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_arnnotlike(std::map<std::string, std::list<std::string>> strings,
                 undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) {
        (void)uv;
        return false;
    };
}

inline matcher
match_null(std::map<std::string, std::list<std::string>> strings) {
    return [strings = std::move(strings)](const http_request&, const command&) {
        return false;
    };
}

} // namespace uh::cluster::ep::policy

#endif
