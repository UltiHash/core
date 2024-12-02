#define BOOST_TEST_MODULE "policy with condition"

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
static std::string
create_test_allow_policy_with_condition(std::string_view sv) {
    auto policy_template = json::parse(R"json({
            "Sid":  "TestCondition",
            "Version": "2012-10-17",
            "Statement": {
                "Effect": "Allow",
                "Action": "*",
                "Principal": "*",
                "Resource": "*"
            }
        })json");
    policy_template["Statement"]["Condition"] = json::parse(sv);

    auto str = policy_template.dump(4);
    return str;
}

/*******************************************************************************
 * Test suite
 */
BOOST_AUTO_TEST_SUITE(allow_policy_with_date_greater_than)

BOOST_AUTO_TEST_CASE(allows_when_current_time_satisfies_the_condition) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateGreaterThan" : {"aws:CurrentTime" : "2020-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::allow);
}

BOOST_AUTO_TEST_CASE(
    doesnt_allow_when_current_time_doesnt_satisfy_the_condition) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateGreaterThan" : {"aws:CurrentTime" : "2220-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END();

/*******************************************************************************
 * Test suite
 */
BOOST_AUTO_TEST_SUITE(allow_policy_with_date_less_than)

BOOST_AUTO_TEST_CASE(allows_when_current_time_satisfies_the_condition) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateLessThan" : {"aws:CurrentTime" : "2220-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::allow);
}

BOOST_AUTO_TEST_CASE(
    doesnt_allow_when_current_time_doesnt_satisfy_the_condition) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateLessThan" : {"aws:CurrentTime" : "2020-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END();

/*******************************************************************************
 * Test suite
 */
BOOST_AUTO_TEST_SUITE(allow_policy_with_two_date_conditions)

BOOST_AUTO_TEST_CASE(allows_when_current_time_satisfies_both_conditions) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateGreaterThan" : {"aws:CurrentTime" : "2020-01-01T00:00:01Z"},
            "DateLessThan" : {"aws:CurrentTime" : "2224-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::allow);
}

BOOST_AUTO_TEST_CASE(
    doesnt_allow_when_current_time_does_not_satisfies_one_of_the_conditions) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateGreaterThan" : {"aws:CurrentTime" : "2020-01-01T00:00:01Z"},
            "DateLessThan" : {"aws:CurrentTime" : "2020-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END();

/*******************************************************************************
 * Test suite
 */
BOOST_AUTO_TEST_SUITE(allow_policy_with_date_not_equals)

BOOST_AUTO_TEST_CASE(doesnt_allow_when_dates_are_equal) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateNotEquals" : {"2011-01-01T00:00:01Z" : "2011-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(
    allows_policy_with_notequals_condition_for_different_dates) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateNotEquals" : {"aws:CurrentTime" : "2011-01-01T00:00:01Z"}
        })json"));

    auto result = policy.front().check(
        variables(make_request("GET /test HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(result.has_value());
    BOOST_CHECK(*result == effect::allow);
}

BOOST_AUTO_TEST_CASE(doesnt_handle_list_type_value) {
    auto policy = parser::parse(create_test_allow_policy_with_condition(
        R"json({
            "DateNotEquals": {
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

BOOST_AUTO_TEST_SUITE_END();
