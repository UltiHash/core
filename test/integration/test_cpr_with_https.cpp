#define BOOST_TEST_MODULE "test cpr with httpbin"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <boost/beast/http/status.hpp>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

BOOST_AUTO_TEST_SUITE(a_cpr)

BOOST_AUTO_TEST_CASE(handles_get_through_https_with_basic_auth) {
    json expected_json = {{"authenticated", true}, {"user", "ultihash"}};

    auto resp = cpr::Get(
        cpr::Url{"https://www.httpbin.org/basic-auth/ultihash/passwd"},
        cpr::Authentication{"ultihash", "passwd", cpr::AuthMode::BASIC});

    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(json::parse(resp.text).dump() == expected_json.dump());
}

BOOST_AUTO_TEST_CASE(handles_post) {

    json expected_json = {
        {"data", "The quick brown fox jumps over the lazy dog"}};

    auto resp =
        cpr::Post(cpr::Url{"http://www.httpbin.org/post"},
                  cpr::Body{"The quick brown fox jumps over the lazy dog"},
                  cpr::Header{{"Content-Type", "text/plain"}});

    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(json::parse(resp.text).at("data").dump() ==
               expected_json.at("data").dump());
}

BOOST_AUTO_TEST_CASE(returns_0_for_post_with_invalid_host) {

    json expected_json = {
        {"data", "The quick brown fox jumps over the lazy dog"}};

    auto resp =
        cpr::Post(cpr::Url{"http://"},
                  cpr::Body{"The quick brown fox jumps over the lazy dog"},
                  cpr::Header{{"Content-Type", "text/plain"}});

    BOOST_TEST(resp.status_code == 0);
}

BOOST_AUTO_TEST_CASE(returns_not_found_for_get_with_invalid_path) {

    json expected_json = {
        {"data", "The quick brown fox jumps over the lazy dog"}};

    auto resp = cpr::Get(
        cpr::Url{"https://www.httpbin.org/wrong_path"},
        cpr::Authentication{"ultihash", "passwd", cpr::AuthMode::BASIC});

    BOOST_TEST(resp.status_code !=
               static_cast<int>(boost::beast::http::status::ok));
}

BOOST_AUTO_TEST_CASE(returns_for_empty_user_and_password) {

    BOOST_CHECK_NO_THROW(
        cpr::Get(cpr::Url{"https:///v1/license"},
                 cpr::Authentication{"", "", cpr::AuthMode::BASIC}));
}

BOOST_AUTO_TEST_SUITE_END()
