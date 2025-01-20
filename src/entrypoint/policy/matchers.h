#pragma once

#include "entrypoint/formats.h"
#include "matcher.h"
#include <set>

namespace uh::cluster::ep::policy {

inline matcher match_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const variables& vars) {
        return match_any(actions, [&vars](auto value) {
            if (auto action = vars.get("uh:ActionId"); action) {
                return equals_wildcard(value, *action);
            }

            throw std::runtime_error("uh:ActionId cannot be evaluated");
        });
    };
}

inline matcher match_not_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const variables& vars) {
        return !match_any(actions, [&vars](auto value) {
            if (auto action = vars.get("uh:ActionId"); action) {
                return equals_wildcard(value, *action);
            }

            throw std::runtime_error("uh:ActionId cannot be evaluated");
        });
    };
}

// We can use policy variables in the `Resource` element and in string
// comparisons in the `Condition` element.
// Predefined policy variables for special charactors can be used in any string
// where you can use regular policy variables.
//
// see
// https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_variables.html#policy-vars-wheretouse
inline matcher match_resource(std::set<std::string> resources) {
    return [resources = std::move(resources)](const variables& vars) {
        return match_any(resources, [&vars](auto value) {
            if (auto arn = vars.get("uh:ResourceArn"); arn) {
                return equals_wildcard(value, *arn, vars);
            }

            throw std::runtime_error("uh:ResourceArn cannot be evaluated");
        });
    };
}

inline matcher match_not_resource(std::set<std::string> resources) {
    return [resources = std::move(resources)](const variables& vars) {
        return !match_any(resources, [&vars](auto value) {
            if (auto arn = vars.get("uh:ResourceArn"); arn) {
                return equals_wildcard(value, *arn, vars);
            }

            throw std::runtime_error("uh:ResourceArn cannot be evaluated");
        });
    };
}

inline matcher match_principal(std::set<std::string> principals) {
    return [principals = std::move(principals)](const variables& vars) {
        return match_any(principals, [&vars](auto value) {
            if (auto arn = vars.get("aws:PrincipalArn"); arn) {
                return equals_wildcard(value, *arn);
            }

            throw std::runtime_error("aws:PrincipalArn cannot be evaluated");
        });
    };
}

inline matcher match_not_principal(std::set<std::string> principals) {
    return [principals = std::move(principals)](const variables& vars) {
        return !match_any(principals, [&vars](auto value) {
            if (auto arn = vars.get("aws:PrincipalArn"); arn) {
                return equals_wildcard(value, *arn);
            }

            throw std::runtime_error("aws:PrincipalArn cannot be evaluated");
        });
    };
}

/*
 * Implements logical AND for multiple context keys attached to a single
 * condition operator
 */
inline matcher var_matcher(std::map<std::string, std::list<std::string>> values,
                           undefined_variable uv, auto match_func) {

    return [values = std::move(values), uv, match_func](const variables& vars) {
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
            return match_any(options, [&](const auto& value) {
                return var == var_replace(value, vars);
            });
        });
}

inline matcher
match_stringnotequals(std::map<std::string, std::list<std::string>> values,
                      undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& value) {
                return var == var_replace(value, vars);
            });
        });
}

inline matcher match_stringequalsignorecase(
    std::map<std::string, std::list<std::string>> values,
    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return match_any(options, [&](const auto& value) {
                return equals_nocase(var, var_replace(value, vars));
            });
        });
}

inline matcher match_stringnotequalsignorecase(
    std::map<std::string, std::list<std::string>> values,
    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& value) {
                return equals_nocase(var, var_replace(value, vars));
            });
        });
}

inline matcher
match_stringlike(std::map<std::string, std::list<std::string>> values,
                 undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return match_any(options, [&](const auto& value) {
                return equals_wildcard(var, value, vars);
            });
        });
}

inline matcher
match_stringnotlike(std::map<std::string, std::list<std::string>> values,
                    undefined_variable uv) {
    return var_matcher(
        std::move(values), uv,
        [](const auto& vars, const auto& var, const auto& options) {
            return !match_any(options, [&](const auto& value) {
                return equals_wildcard(var, value, vars);
            });
        });
}

template <typename Comparator>
inline matcher
match_numericcomparison(std::map<std::string, std::list<std::string>> values,
                        undefined_variable uv, Comparator comp) {
    return var_matcher(
        std::move(values), uv,
        [comp](const auto& vars, const auto& var, const auto& options) {
            if (options.size() != 1) [[unlikely]]
                throw std::runtime_error("list is not supported as a condition "
                                         "value for this comparison operator");
            return comp(to_int(var),
                        to_int(var_replace(options.front(), vars)));
        });
}

template <typename Comparator>
inline matcher
match_datecomparison(std::map<std::string, std::list<std::string>> values,
                     undefined_variable uv, Comparator comp) {
    using namespace std::chrono_literals;
    return var_matcher(
        std::move(values), uv,
        [comp](const auto& vars, const auto& var, const auto& options) -> bool {
            if (options.size() != 1) [[unlikely]]
                throw std::runtime_error("list is not supported as a condition "
                                         "value for this comparison operator");
            return comp(read_iso8601_date(var),
                        read_iso8601_date(var_replace(options.front(), vars)));
        });
}

inline matcher match_bool(std::map<std::string, std::list<std::string>> strings,
                          undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("Bool not implemented");
    };
}

inline matcher
match_binaryequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("BinaryEquals not implemented");
    };
}

inline matcher
match_ipaddress(std::map<std::string, std::list<std::string>> strings,
                undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("IpAddress not implemented");
    };
}

inline matcher
match_notipaddress(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("NotIpAddress not implemented");
    };
}

inline matcher
match_arnequals(std::map<std::string, std::list<std::string>> strings,
                undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnEquals not implemented");
    };
}

inline matcher
match_arnlike(std::map<std::string, std::list<std::string>> strings,
              undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnLike not implemented");
    };
}

inline matcher
match_arnnotequals(std::map<std::string, std::list<std::string>> strings,
                   undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnNotEquals not implemented");
    };
}

inline matcher
match_arnnotlike(std::map<std::string, std::list<std::string>> strings,
                 undefined_variable uv) {
    return [strings = std::move(strings), uv](const variables&) -> bool {
        (void)uv;
        throw std::runtime_error("ArnNotLike not implemented");
    };
}

inline matcher
match_null(std::map<std::string, std::list<std::string>> strings) {
    return [strings = std::move(strings)](const variables&) -> bool {
        throw std::runtime_error("Null not implemented");
    };
}

} // namespace uh::cluster::ep::policy
