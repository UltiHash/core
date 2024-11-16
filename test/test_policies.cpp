#define BOOST_TEST_MODULE "policy"

#include "entrypoint.h"
#include "entrypoint/policy/parser.h"
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

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
