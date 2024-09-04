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

std::set<std::string> string_or_set(const json& element) {
    if (element.is_array()) {
        return std::set<std::string>(element.begin(), element.end());
    }

    return {element.get<std::string>()};
}

action get_action(const json& stmt) {
    auto effect = require(stmt, "Effect").get<std::string>();
    if (effect == "Allow") {
        return action::allow;
    }

    if (effect == "Deny") {
        return action::deny;
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

std::optional<matcher> condition_matchers(const json& stmt) {
    if (auto condition = optional(stmt, "Condition"); condition) {
        // TODO add condition support
    }

    return {};
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

    return policy(id, std::move(matchers), get_action(stmt));
}

} // namespace

std::list<policy> parser::parse(const std::string& code) {
    auto js = json::parse(code);

    auto version = optional(js, "Version");
    if (!version || version->get().get<std::string>() != IAM_JSON_VERSION) {
        throw std::runtime_error("no version element or unsupported version");
    }

    std::list<policy> rv;

    const auto& statements = require(js, "Statement");
    LOG_DEBUG() << "policy parser: " << statements.size()
                << " policies in Statement";
    if (statements.is_array()) {
        for (const auto& stmt : statements) {
            rv.emplace_back(parse_policy(stmt));
        }
    } else {
        rv.emplace_back(parse_policy(statements));
    }

    return rv;
}

} // namespace uh::cluster::ep::policy
