#define BOOST_TEST_MODULE "policy with action"

#include "entrypoint.h"
#include "entrypoint/policy/parser.h"
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

// ------------- Tests Suites Follow --------------

using namespace uh::cluster;
using namespace uh::cluster::ep::policy;
using namespace uh::cluster::ep::user;
using namespace uh::cluster::test;

using json = nlohmann::json;

static std::string create_allow_policy(std::string_view key,
                                       std::string_view value) {
    auto policy_template = json::parse(R"json({
            "Sid":  "TestCondition",
            "Version": "2012-10-17",
            "Statement": {
                "Effect": "Allow",
                "Principal": "*",
                "Resource": "*"
            }
        })json");
    policy_template["Statement"][key] = json::parse(value);

    auto str = policy_template.dump(4);
    return str;
}

/*******************************************************************************
 * Test suite
 */
BOOST_AUTO_TEST_SUITE(allow_policy_with_action)

BOOST_AUTO_TEST_CASE(allows_one_of_the_allowed_actions) {
    auto policy = parser::parse(create_allow_policy(
        "Action", R"json(["s3:GetObject", "s3:PutObject"])json"));

    auto get_result = policy.front().check(
        variables(make_request("GET /test/obj.txt HTTP/1.1\r\n\r\n"),
                  mock_command("s3:GetObject")));
    auto put_result = policy.front().check(
        variables(make_request("PUT /test/obj.txt HTTP/1.1\r\n\r\n"),
                  mock_command("s3:PutObject")));

    BOOST_CHECK(get_result.has_value());
    BOOST_CHECK(*get_result == effect::allow);
    BOOST_CHECK(put_result.has_value());
    BOOST_CHECK(*put_result == effect::allow);
}

BOOST_AUTO_TEST_CASE(doesnt_allow_not_allowed_actions) {
    auto policy = parser::parse(create_allow_policy(
        "Action", R"json(["s3:GetObject", "s3:PutObject"])json"));

    auto list_result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(!list_result.has_value());
}

BOOST_AUTO_TEST_CASE(allows_every_action_with_wildcard_match) {
    auto policy =
        parser::parse(create_allow_policy("Action", R"json("s3:*")json"));

    auto put_result = policy.front().check(
        variables(make_request("PUT /test/obj.txt HTTP/1.1\r\n\r\n"),
                  mock_command("s3:PutObject")));
    auto get_result = policy.front().check(
        variables(make_request("GET /test/obj.txt HTTP/1.1\r\n\r\n"),
                  mock_command("s3:GetObject")));
    auto list_result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(put_result.has_value());
    BOOST_CHECK(*put_result == effect::allow);
    BOOST_CHECK(get_result.has_value());
    BOOST_CHECK(*get_result == effect::allow);
    BOOST_CHECK(list_result.has_value());
    BOOST_CHECK(*list_result == effect::allow);
}

BOOST_AUTO_TEST_SUITE_END()

/*******************************************************************************
 * Test suite
 */
BOOST_AUTO_TEST_SUITE(allow_policy_with_not_action)

BOOST_AUTO_TEST_CASE(allows_not_mentioned_actions) {
    auto policy = parser::parse(create_allow_policy(
        "NotAction", R"json(["s3:GetObject", "s3:PutObject"])json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::allow);
    // BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(doesnt_allow_mentioned_actions) {
    auto policy = parser::parse(create_allow_policy(
        "NotAction", R"json(["s3:GetObject", "s3:PutObject"])json"));

    auto get_result = policy.front().check(
        variables(make_request("GET /test/obj.txt HTTP/1.1\r\n\r\n"),
                  mock_command("s3:GetObject")));
    auto put_result = policy.front().check(
        variables(make_request("PUT /test/obj.txt HTTP/1.1\r\n\r\n"),
                  mock_command("s3:PutObject")));

    BOOST_CHECK(!get_result.has_value());
    BOOST_CHECK(!put_result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
