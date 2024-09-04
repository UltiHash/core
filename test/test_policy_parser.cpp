#define BOOST_TEST_MODULE "policy JSON parser tests"

#include "entrypoint/policy/parser.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

using namespace uh::cluster::ep::policy;

BOOST_AUTO_TEST_CASE(parse_success) {
    auto policies = parser::parse(
        "{\n"
        "   \"Version\": \"2012-10-17\",\n"
        "   \"Statement\": [{\n"
        "       \"Sid\": \"AllowRemoveMfaOnlyIfRecentMfa\",\n"
        "       \"Effect\": \"Allow\",\n"
        "       \"Action\": [\n"
        "           \"iam:DeactivateMFADevice\"\n"
        "       ],\n"
        "       \"Principal\": \"\",\n"
        "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\",\n"
        "       \"Condition\": {\n"
        "           \"NumericLessThanEquals\": {\"aws:MultiFactorAuthAge\": "
        "\"3600\"}\n"
        "       }\n"
        "   }]\n"
        "}\n");

    BOOST_CHECK(policies.size() == 1);

    const auto& policy = policies.front();
    BOOST_CHECK(policy.id() == "AllowRemoveMfaOnlyIfRecentMfa");
    BOOST_CHECK(policy.effect() == action::allow);
}

BOOST_AUTO_TEST_CASE(parse_success_negation) {
    auto policies = parser::parse(
        "{\n"
        "   \"Version\": \"2012-10-17\",\n"
        "   \"Statement\": [{\n"
        "       \"Sid\": \"AllowRemoveMfaOnlyIfRecentMfa\",\n"
        "       \"Effect\": \"Allow\",\n"
        "       \"NotAction\": [\n"
        "           \"iam:DeactivateMFADevice\"\n"
        "       ],\n"
        "       \"NotPrincipal\": \"\",\n"
        "       \"NotResource\": \"arn:aws:iam::*:user/${aws:username}\",\n"
        "       \"Condition\": {\n"
        "           \"NumericLessThanEquals\": {\"aws:MultiFactorAuthAge\": "
        "\"3600\"}\n"
        "       }\n"
        "   }]\n"
        "}\n");

    BOOST_CHECK(policies.size() == 1);

    const auto& policy = policies.front();
    BOOST_CHECK(policy.id() == "AllowRemoveMfaOnlyIfRecentMfa");
    BOOST_CHECK(policy.effect() == action::allow);
}

BOOST_AUTO_TEST_CASE(parse_single_statement) {
    auto policies = parser::parse(
        "{\n"
        "   \"Version\": \"2012-10-17\",\n"
        "   \"Statement\": {\n"
        "       \"Sid\": \"AllowRemoveMfaOnlyIfRecentMfa\",\n"
        "       \"Effect\": \"Allow\",\n"
        "       \"Action\": [\n"
        "           \"iam:DeactivateMFADevice\"\n"
        "       ],\n"
        "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\",\n"
        "       \"Condition\": {\n"
        "           \"NumericLessThanEquals\": {\"aws:MultiFactorAuthAge\": "
        "\"3600\"}\n"
        "       }\n"
        "   }\n"
        "}\n");

    BOOST_CHECK(policies.size() == 1);

    const auto& policy = policies.front();
    BOOST_CHECK(policy.id() == "AllowRemoveMfaOnlyIfRecentMfa");
    BOOST_CHECK(policy.effect() == action::allow);
}

BOOST_AUTO_TEST_CASE(parse_error_version) {
    BOOST_CHECK_THROW(
        parser::parse(
            "{\n"
            "   \"Version\": \"unsupported version\",\n"
            "   \"Statement\": {\n"
            "       \"Effect\": \"Allow\",\n"
            "       \"Action\": [],\n"
            "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\",\n"
            "   }\n"
            "}\n"),
        std::exception);
}

BOOST_AUTO_TEST_CASE(parse_error_resource_missing) {
    BOOST_CHECK_THROW(parser::parse("{\n"
                                    "   \"Version\": \"2012-10-17\",\n"
                                    "   \"Statement\": {\n"
                                    "       \"Effect\": \"Allow\",\n"
                                    "       \"Action\": [],\n"
                                    "   }\n"
                                    "}\n"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(parse_error_resource_conflict) {
    BOOST_CHECK_THROW(parser::parse("{\n"
                                    "   \"Version\": \"2012-10-17\",\n"
                                    "   \"Statement\": {\n"
                                    "       \"Effect\": \"Allow\",\n"
                                    "       \"Action\": [],\n"
                                    "       \"Resource\": \"\",\n"
                                    "       \"NotResource\": \"\"\n"
                                    "   }\n"
                                    "}\n"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(parse_error_action_missing) {
    BOOST_CHECK_THROW(
        parser::parse(
            "{\n"
            "   \"Version\": \"2012-10-17\",\n"
            "   \"Statement\": {\n"
            "       \"Effect\": \"Allow\",\n"
            "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\"\n"
            "   }\n"
            "}\n"),
        std::exception);
}

BOOST_AUTO_TEST_CASE(parse_error_action_conflict) {
    BOOST_CHECK_THROW(
        parser::parse(
            "{\n"
            "   \"Version\": \"2012-10-17\",\n"
            "   \"Statement\": {\n"
            "       \"Action\": [],\n"
            "       \"NotAction\": [],\n"
            "       \"Effect\": \"Allow\",\n"
            "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\"\n"
            "   }\n"
            "}\n"),
        std::exception);
}

BOOST_AUTO_TEST_CASE(parse_error_effect_missing) {
    BOOST_CHECK_THROW(
        parser::parse(
            "{\n"
            "   \"Version\": \"2012-10-17\",\n"
            "   \"Statement\": {\n"
            "       \"Action\": [],\n"
            "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\"\n"
            "   }\n"
            "}\n"),
        std::exception);
}

BOOST_AUTO_TEST_CASE(parse_error_effect_unknown) {
    BOOST_CHECK_THROW(
        parser::parse(
            "{\n"
            "   \"Version\": \"2012-10-17\",\n"
            "   \"Statement\": {\n"
            "       \"Effect\": \"Unkown Effect\",\n"
            "       \"Action\": [],\n"
            "       \"Resource\": \"arn:aws:iam::*:user/${aws:username}\"\n"
            "   }\n"
            "}\n"),
        std::exception);
}
