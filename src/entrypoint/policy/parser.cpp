#include "parser.h"

#include "matcher.h"

#include "common/telemetry/log.h"
#include <functional>
#include <nlohmann/json.hpp>
#include <set>
#include <string>

using namespace nlohmann;

namespace uh::cluster::ep::policy {

namespace {

const json& require(const json& j, std::string_view key) {
    auto it = j.find(key);
    if (it == j.end()) {
        throw std::runtime_error("required key `" + std::string(key) +
                                 "` not found");
    }

    return *it;
}

std::optional<std::reference_wrapper<const json>>
optional(const json& j, std::string_view key) {

    auto it = j.find(key);
    if (it == j.end()) {
        return {};
    }

    return *it;
}

std::string to_string(const json& element) {
    return element.get<std::string>();
}

template <class container>
auto multi_element(const json& element, auto reader) {
    if (!element.is_array()) {
        return container{reader(element)};
    }

    container rv;
    for (const auto& sub : element) {
        rv.insert(rv.end(), reader(sub));
    }
    return rv;
}

std::set<std::string> string_or_set(const json& element) {
    return multi_element<std::set<std::string>>(element, to_string);
}

effect get_effect(const json& stmt) {
    auto effect = require(stmt, "Effect").get<std::string>();
    if (effect == "Allow") {
        return effect::allow;
    }

    if (effect == "Deny") {
        return effect::deny;
    }

    throw std::runtime_error("unsupported effect value");
}

matcher action_matchers(const json& stmt) {
    auto action = optional(stmt, "Action");
    auto not_action = optional(stmt, "NotAction");

    if (action && not_action) {
        throw std::runtime_error("Action and NotAction both are defined");
    }

    if (action) {
        return match_action(string_or_set(action->get()));
    }

    if (not_action) {
        return match_not_action(string_or_set(not_action->get()));
    }

    throw std::runtime_error("either Action or NotAction is required");
}

matcher resource_matchers(const json& stmt) {
    auto resource = optional(stmt, "Resource");
    auto not_resource = optional(stmt, "NotResource");

    if (resource && not_resource) {
        throw std::runtime_error("Resource and NotResource both are defined");
    }

    if (resource) {
        return match_resource(string_or_set(resource->get()));
    }

    if (not_resource) {
        return match_not_resource(string_or_set(not_resource->get()));
    }

    throw std::runtime_error("either Resource or NotResource is required");
}

std::optional<matcher> principal_matchers(const json& stmt) {
    if (auto principal = optional(stmt, "Principal"); principal) {
        return match_principal(string_or_set(principal->get()));
    }

    if (auto not_principal = optional(stmt, "NotPrincipal"); not_principal) {
        return match_not_principal(string_or_set(not_principal->get()));
    }

    return {};
}

std::map<std::string, std::list<std::string>>
condition_parameter(const json& condition) {
    std::map<std::string, std::list<std::string>> rv;
    for (const auto& elem : condition.items()) {
        rv[elem.key()] =
            multi_element<std::list<std::string>>(elem.value(), to_string);
    }
    return rv;
}

matcher condition_matcher(std::string key, const json& condition) {
    undefined_variable if_exists = undefined_variable::do_not_match;
    if (key.ends_with("IfExists")) {
        if_exists = undefined_variable::ignore;
        key = key.substr(0, key.size() - std::string("IfExists").size());
    }

    if (key == "StringEquals") {
        return match_stringequals(condition_parameter(condition), if_exists);
    }
    if (key == "StringNotEquals") {
        return match_stringnotequals(condition_parameter(condition), if_exists);
    }
    if (key == "StringEqualsIgnoreCase") {
        return match_stringequalsignorecase(condition_parameter(condition),
                                            if_exists);
    }
    if (key == "StringNotEqualsIgnoreCase") {
        return match_stringnotequalsignorecase(condition_parameter(condition),
                                               if_exists);
    }
    if (key == "StringLike") {
        return match_stringlike(condition_parameter(condition), if_exists);
    }
    if (key == "StringNotLike") {
        return match_stringnotlike(condition_parameter(condition), if_exists);
    }
    if (key == "NumericEquals") {
        return match_numericequals(condition_parameter(condition), if_exists);
    }
    if (key == "NumericNotEquals") {
        return match_numericnotequals(condition_parameter(condition),
                                      if_exists);
    }
    if (key == "NumericLessThan") {
        return match_numericlessthan(condition_parameter(condition), if_exists);
    }
    if (key == "NumericLessThanEquals") {
        return match_numericlessthanequals(condition_parameter(condition),
                                           if_exists);
    }
    if (key == "NumericGreaterThan") {
        return match_numericgreaterthan(condition_parameter(condition),
                                        if_exists);
    }
    if (key == "NumericGreaterThanEquals") {
        return match_numericgreaterthanequals(condition_parameter(condition),
                                              if_exists);
    }
    if (key == "DateEquals") {
        return match_dateequals(condition_parameter(condition), if_exists);
    }
    if (key == "DateNotEquals") {
        return match_datenotequals(condition_parameter(condition), if_exists);
    }
    if (key == "DateLessThan") {
        return match_datelessthan(condition_parameter(condition), if_exists);
    }
    if (key == "DateLessThanEquals") {
        return match_datelessthanequals(condition_parameter(condition),
                                        if_exists);
    }
    if (key == "DateGreaterThan") {
        return match_dategreaterthan(condition_parameter(condition), if_exists);
    }
    if (key == "DateGreaterThanEquals") {
        return match_dategreaterthanequals(condition_parameter(condition),
                                           if_exists);
    }
    if (key == "Bool") {
        return match_bool(condition_parameter(condition), if_exists);
    }
    if (key == "BinaryEquals") {
        return match_binaryequals(condition_parameter(condition), if_exists);
    }
    if (key == "IpAddress") {
        return match_ipaddress(condition_parameter(condition), if_exists);
    }
    if (key == "NotIpAddress") {
        return match_notipaddress(condition_parameter(condition), if_exists);
    }
    if (key == "ArnEquals") {
        return match_arnequals(condition_parameter(condition), if_exists);
    }
    if (key == "ArnLike") {
        return match_arnlike(condition_parameter(condition), if_exists);
    }
    if (key == "ArnNotEquals") {
        return match_arnnotequals(condition_parameter(condition), if_exists);
    }
    if (key == "ArnNotLike") {
        return match_arnnotlike(condition_parameter(condition), if_exists);
    }
    if (key == "Null") {
        return match_null(condition_parameter(condition));
    }

    return match_never();
}

std::optional<matcher> condition_matchers(const json& stmt) {
    auto condition = optional(stmt, "Condition");
    if (!condition) {
        return {};
    }

    std::list<matcher> subs;

    for (const auto& elem : condition->get().items()) {
        subs.emplace_back(condition_matcher(elem.key(), elem.value()));
    }

    return match_all(std::move(subs));
}

policy parse_policy(const json& stmt) {

    std::list<matcher> matchers;

    matchers.emplace_back(action_matchers(stmt));
    matchers.emplace_back(resource_matchers(stmt));

    if (auto matcher = principal_matchers(stmt); matcher) {
        matchers.emplace_back(std::move(*matcher));
    }

    if (auto matcher = condition_matchers(stmt); matcher) {
        matchers.emplace_back(std::move(*matcher));
    }

    std::string id = parser::UNDEFINED_SID;
    auto sid = optional(stmt, "Sid");
    if (sid) {
        id = sid->get().get<std::string>();
    }

    return policy(id, std::move(matchers), get_effect(stmt));
}

} // namespace

std::list<policy> parser::parse(const std::string& code) {
    auto js = json::parse(code);

    auto version = optional(js, "Version");
    if (!version || version->get().get<std::string>() != IAM_JSON_VERSION) {
        throw std::runtime_error("no version element or unsupported version");
    }

    const auto& statements = require(js, "Statement");
    LOG_DEBUG() << "policy parser: " << statements.size()
                << " policies in Statement";

    return multi_element<std::list<policy>>(statements, parse_policy);
}

} // namespace uh::cluster::ep::policy
