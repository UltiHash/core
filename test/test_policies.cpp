#define BOOST_TEST_MODULE "policy JSON parser tests"

#include "entrypoint/policy/parser.h"
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>
#include <test/entrypoint.h>

// ------------- Tests Suites Follow --------------

using namespace uh::cluster;
using namespace uh::cluster::ep::policy;
using namespace uh::cluster::ep::user;
using namespace uh::cluster::test;

BOOST_AUTO_TEST_CASE(check_action) {
    auto policy = parser::parse("{\n"
                                "   \"Version\": \"2012-10-17\",\n"
                                "   \"Statement\": {\n"
                                "       \"Sid\": \"AllowAllForGetObject\",\n"
                                "       \"Effect\": \"Allow\",\n"
                                "       \"Action\": \"s3:GetObject\",\n"
                                "       \"Principal\": \"*\",\n"
                                "       \"Resource\": \"*\"\n"
                                "   }\n"
                                "}\n");
    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request = make_request("GET /my_object_id HTTP/1.1\r\n"
                                    "Host: bucket-id\r\n"
                                    "\r\n");

        auto result = policy.front().check(
            variables(request, mock_command("s3:GetObject")));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }

    {
        auto request = make_request("GET /my_object_id HTTP/1.1\r\n"
                                    "Host: bucket-id\r\n"
                                    "\r\n");

        auto result = policy.front().check(
            variables(request, mock_command("s3:PutObject")));
        BOOST_CHECK(!result.has_value());
    }
}

BOOST_AUTO_TEST_CASE(check_principal) {
    auto policy = parser::parse("{\n"
                                "   \"Version\": \"2012-10-17\",\n"
                                "   \"Statement\": {\n"
                                "       \"Sid\": \"AllowAnonForGetObject\",\n"
                                "       \"Effect\": \"Allow\",\n"
                                "       \"Action\": \"s3:GetObject\",\n"
                                "       \"Principal\": \"" +
                                user::ANONYMOUS_ARN +
                                "\",\n"
                                "       \"Resource\": \"*\"\n"
                                "   }\n"
                                "}\n");
    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request =
            make_request("GET /bucket/my_object_id HTTP/1.1\r\n\r\n");
        auto result = policy.front().check(
            variables(request, mock_command("s3:GetObject")));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }

    {
        auto request = make_request("GET /bucket/my_object_id HTTP/1.1\r\n\r\n",
                                    "arn:aws:iam::2:random_user");

        auto result = policy.front().check(
            variables(request, mock_command("s3:GetObject")));
        BOOST_CHECK(!result.has_value());
    }
}

BOOST_AUTO_TEST_CASE(check_resource) {
    auto policy =
        parser::parse("{\n"
                      "   \"Version\": \"2012-10-17\",\n"
                      "   \"Statement\": {\n"
                      "       \"Sid\": \"AllowAllForResource\",\n"
                      "       \"Effect\": \"Allow\",\n"
                      "       \"Action\": \"s3:GetObject\",\n"
                      "       \"Principal\": \"*\",\n"
                      "       \"Resource\": \"arn:aws:s3:::bucket/*\"\n"
                      "   }\n"
                      "}\n");
    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request =
            make_request("GET /bucket/my_object_id HTTP/1.1\r\n\r\n");
        auto result = policy.front().check(
            variables(request, mock_command("s3:GetObject")));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }

    {
        auto request = make_request("GET /vedro/my_object_id HTTP/1.1\r\n\r\n");
        auto result = policy.front().check(
            variables(request, mock_command("s3:GetObject")));
        BOOST_CHECK(!result.has_value());
    }
}

BOOST_AUTO_TEST_CASE(check_allow_all_policy) {
    auto policy = parser::parse("{\n"
                                "  \"Version\": \"2012-10-17\",\n"
                                "  \"Statement\": {\n"
                                "    \"Sid\":  \"AllowAllForAnybody\",\n"
                                "    \"Effect\": \"Allow\",\n"
                                "    \"Action\": \"*\",\n"
                                "    \"Principal\": \"*\",\n"
                                "    \"Resource\": \"*\"\n"
                                "  }\n"
                                "}\n");

    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request = make_request("GET /test HTTP/1.1\r\n\r\n");
        auto result = policy.front().check(
            variables(request, mock_command("s3:ListBucket")));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }
}

using json = nlohmann::json;
static std::string create_test_deny_policy_with_condition(std::string_view sv) {
    auto policy_template = json::parse(R"json({
            "Sid":  "TestCondition",
            "Version": "2012-10-17",
            "Statement": {
                "Effect": "Deny",
                "Action": "*",
                "Principal": "*",
                "Resource": "*",
                "Condition": {}
            }
        })json");
    policy_template["Statement"]["Condition"] = json::parse(sv);

    auto str = policy_template.dump(4);
    return str;
}

BOOST_AUTO_TEST_CASE(check_condition__handles_lessthan_and_greaterthan) {
    auto policy = parser::parse(create_test_deny_policy_with_condition(
        R"json({
            "DateGreaterThan" : {"aws:CurrentTime" : "2020-01-01T00:00:01Z"},
            "DateLessThan" : {"aws:CurrentTime" : "2200-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::deny);
}

BOOST_AUTO_TEST_CASE(
    check_condition__do_not_add_deny_policy_with_equals_condition_for_different_dates) {
    auto policy = parser::parse(create_test_deny_policy_with_condition(
        R"json({
            "DateEquals" : {"aws:CurrentTime" : "2011-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(
    check_condition__adds_deny_policy_with_notequals_condition_for_different_dates) {
    auto policy = parser::parse(create_test_deny_policy_with_condition(
        R"json({
            "DateNotEquals" : {"aws:CurrentTime" : "2011-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::deny);
}

BOOST_AUTO_TEST_CASE(check_condition__does_not_handle_list_type_value) {
    auto policy = parser::parse(create_test_deny_policy_with_condition(
        R"json({
            "DateGreaterThan": {
                "aws:CurrentTime": [
                    "2020-01-01T00:00:01Z", "2020-01-01T00:00:01Z"
                ]
            }
        })json"));

    BOOST_REQUIRE_THROW(policy.front().check(variables(
                            make_request("GET /test HTTP/1.1\r\n\r\n"),
                            mock_command("s3:ListBucket"))),
                        std::runtime_error);
}
