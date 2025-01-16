#include <list>
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

/*
 * See
 * https://docs.aws.amazon.com/AmazonS3/latest/userguide/example-policies-s3.html
 */

/*******************************************************************************
 * Test suite
 */
struct common_policy_for_single_bucket {
    common_policy_for_single_bucket()
        : policies{define_policies()} {}
    ~common_policy_for_single_bucket() {}

    static std::list<policy> define_policies() {
        auto policy_json = json::parse(R"json(
        {
          "Version": "2012-10-17",
          "Statement": [
            {
              "Effect": "Allow",
              "Action": ["s3:ListBucket"],
              "Resource": ["arn:aws:s3:::test"]
            },
            {
              "Effect": "Allow",
              "Action": [
                "s3:PutObject",
                "s3:GetObject",
                "s3:DeleteObject"
              ],
              "Resource": ["arn:aws:s3:::test/*"]
            }
          ]
        })json");
        return parser::parse(policy_json.dump(2));
    }

    std::list<policy> policies;
};

BOOST_FIXTURE_TEST_SUITE(common_policy, common_policy_for_single_bucket)

BOOST_AUTO_TEST_CASE(allows_listbucket_on_bucket) {

    auto effect = policies.front().check(
        variables(make_request("GET /test/?list-type=2 HTTP/1.1\r\n\r\n"),
                  mock_command("s3:ListBucket")));

    BOOST_CHECK(effect.has_value());
    BOOST_CHECK(*effect == effect::allow);
}

BOOST_AUTO_TEST_CASE(allows_putobject_into_bucket) {
    auto effect = std::next(policies.begin())
                      ->check(variables(
                          make_request("PUT /test/obj.txt HTTP/1.1\r\n\r\n"),
                          mock_command("s3:PutObject")));

    BOOST_CHECK(effect.has_value());
    BOOST_CHECK(*effect == effect::allow);
}

BOOST_AUTO_TEST_CASE(allows_getobject_into_bucket) {
    auto effect = std::next(policies.begin())
                      ->check(variables(
                          make_request("GET /test/obj.txt HTTP/1.1\r\n\r\n"),
                          mock_command("s3:GetObject")));

    BOOST_CHECK(effect.has_value());
    BOOST_CHECK(*effect == effect::allow);
}

BOOST_AUTO_TEST_CASE(allows_deleteobject_into_bucket) {
    auto effect = std::next(policies.begin())
                      ->check(variables(
                          make_request("DELETE /test/obj.txt HTTP/1.1\r\n\r\n"),
                          mock_command("s3:DeleteObject")));

    BOOST_CHECK(effect.has_value());
    BOOST_CHECK(*effect == effect::allow);
}

BOOST_AUTO_TEST_CASE(denies_deleteobject_on_bucket) {
    auto effect =
        std::next(policies.begin())
            ->check(variables(make_request("DELETE /test HTTP/1.1\r\n\r\n"),
                              mock_command("s3:DeleteObject")));

    BOOST_CHECK(!effect.has_value());
}

BOOST_AUTO_TEST_SUITE_END();
