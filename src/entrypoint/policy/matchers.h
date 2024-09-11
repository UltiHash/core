#ifndef CORE_ENTRYPOINT_POLICY_MATCHERS_H
#define CORE_ENTRYPOINT_POLICY_MATCHERS_H

#include "entrypoint/commands/command.h"
#include "entrypoint/http/http_request.h"
#include "entrypoint/variables.h"
#include "matcher.h"
#include <iostream>

namespace uh::cluster::ep::policy {

inline matcher match_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const http_request&,
                                          const command& cmd) {
        return actions.find(cmd.action_id()) != actions.end();
    };
}

inline matcher match_not_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const http_request&,
                                          const command& cmd) {
        return actions.find(cmd.action_id()) == actions.end();
    };
}

inline matcher match_resource(std::set<std::string> resources) {
    return [resources = std::move(resources)](const http_request& r,
                                              const command& cmd) {
        std::string arn = "arn:aws:s3:::" + r.bucket() + "/" + r.object_key();

        return match_any(resources, [&arn](auto value) {
            return equals_wildcard(value, arn);
        });
    };
}

inline matcher match_not_resource(std::set<std::string> resources) {
    return [resources = std::move(resources)](const http_request& r,
                                              const command& cmd) {
        std::string arn = "arn:aws:s3:::" + r.bucket() + "/" + r.object_key();

        return match_all(resources, [&arn](auto value) {
            return !equals_wildcard(value, arn);
        });
    };
}

inline matcher match_principal(std::set<std::string> principals) {
    return [principals = std::move(principals)](const http_request& r,
                                                const command& cmd) {
        return match_any(principals, [&r](auto value) {
            return equals_wildcard(value, r.authenticated_user().arn);
        });
    };
}

inline matcher match_not_principal(std::set<std::string> principals) {
    return [principals = std::move(principals)](const http_request& r,
                                                const command& cmd) {
        return match_all(principals, [&r](auto value) {
            return !equals_wildcard(value, r.authenticated_user().arn);
        });
    };
}

inline matcher var_matcher(std::map<std::string, std::list<std::string>> values,
                           undefined_variable uv, auto match_func) {

    return [values = std::move(values), uv, match_func](const http_request& r,
                                                        const command&) {
        const variables& vars = r.vars();
        for (const auto& check : values) {
            auto value = vars.get(check.first);
            if (!value) {
                if (uv == undefined_variable::ignore) {
                    continue;
                } else {
                    return false;
                }
            }

            if (!match_func(vars, *value, check.second)) {
                return false;
            }
        }

        return true;
    };
}

inline matcher
match_stringequals(std::map<std::string, std::list<std::string>> values,
                   undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return match_any(options, [&](const auto& v) {
                return var == var_replace(v, vars);
            });
        });
}

inline matcher
match_stringnotequals(std::map<std::string, std::list<std::string>> values,
                      undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return var == var_replace(v, vars);
            });
        });
}

inline matcher match_stringequalsignorecase(
    std::map<std::string, std::list<std::string>> values,
    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return match_any(options, [&](const auto& v) {
                return equals_nocase(var, var_replace(v, vars));
            });
        });
}

inline matcher match_stringnotequalsignorecase(
    std::map<std::string, std::list<std::string>> values,
    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return equals_nocase(var, var_replace(v, vars));
            });
        });
}

inline matcher
match_stringlike(std::map<std::string, std::list<std::string>> values,
                 undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return match_any(options, [&](const auto& v) {
                return equals_wildcard(var, var_replace(v, vars));
            });
        });
}

inline matcher
match_stringnotlike(std::map<std::string, std::list<std::string>> values,
                    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return equals_wildcard(var, var_replace(v, vars));
            });
        });
}

inline matcher
match_numericequals(std::map<std::string, std::list<std::string>> values,
                    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return match_any(options, [&](const auto& v) {
                return to_int(var) == to_int(var_replace(v, vars));
            });
        });
}

inline matcher
match_numericnotequals(std::map<std::string, std::list<std::string>> values,
                       undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return to_int(var) == to_int(var_replace(v, vars));
            });
        });
}

inline matcher
match_numericlessthan(std::map<std::string, std::list<std::string>> values,
                      undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return to_int(var) < to_int(var_replace(v, vars));
            });
        });
}

inline matcher match_numericlessthanequals(
    std::map<std::string, std::list<std::string>> values,
    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return to_int(var) <= to_int(var_replace(v, vars));
            });
        });
}

inline matcher
match_numericgreaterthan(std::map<std::string, std::list<std::string>> values,
                         undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return to_int(var) > to_int(var_replace(v, vars));
            });
        });
}

inline matcher match_numericgreaterthanequals(
    std::map<std::string, std::list<std::string>> values,
    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& v) {
                return to_int(var) >= to_int(var_replace(v, vars));
            });
        });
}

inline matcher
match_dateequals(std::map<std::string, std::list<std::string>> strings,
                 undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("DateEquals not implemented");
    };
}

inline matcher
match_datenotequals(std::map<std::string, std::list<std::string>> strings,
                    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("DateNotEquals not implemented");
    };
}

inline matcher
match_datelessthan(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("DateLessThan not implemented");
    };
}

inline matcher
match_datelessthanequals(std::map<std::string, std::list<std::string>> strings,
                         undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("DateLessThanEquals not implemented");
    };
}

inline matcher
match_dategreaterthan(std::map<std::string, std::list<std::string>> strings,
                      undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("DateGreaterThan not implemented");
    };
}

inline matcher match_dategreaterthanequals(
    std::map<std::string, std::list<std::string>> strings,
    undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("DateGreaterThanEquals not implemented");
    };
}

inline matcher match_bool(std::map<std::string, std::list<std::string>> strings,
                          undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("Bool not implemented");
    };
}

inline matcher
match_binaryequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("BinaryEquals not implemented");
    };
}

inline matcher
match_ipaddress(std::map<std::string, std::list<std::string>> strings,
                undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("IpAddress not implemented");
    };
}

inline matcher
match_notipaddress(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("NotIpAddress not implemented");
    };
}

inline matcher
match_arnequals(std::map<std::string, std::list<std::string>> strings,
                undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnEquals not implemented");
    };
}

inline matcher
match_arnlike(std::map<std::string, std::list<std::string>> strings,
              undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnLike not implemented");
    };
}

inline matcher
match_arnnotequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnNotEquals not implemented");
    };
}

inline matcher
match_arnnotlike(std::map<std::string, std::list<std::string>> strings,
                 undefined_variable uv) {
    return [strings = std::move(strings), uv](const http_request&,
                                              const command&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnNotLike not implemented");
    };
}

inline matcher
match_null(std::map<std::string, std::list<std::string>> strings) {
    return [strings = std::move(strings)](const http_request& r,
                                          const command&) -> bool {
        throw std::runtime_error("Null not implemented");
    };
}

} // namespace uh::cluster::ep::policy

#endif
