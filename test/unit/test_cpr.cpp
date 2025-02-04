#define BOOST_TEST_MODULE "license tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <boost/beast/http/status.hpp>
#include <cpr/cpr.h>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpr/ssl_options.h>
#include <httplib.h>
#include <lib/mock/http_server/http_server.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

BOOST_AUTO_TEST_SUITE(a_cpr)

BOOST_AUTO_TEST_CASE(handles_get_through_https_with_basic_auth) {
    auto server = http_server("test_user", "password");
    std::string expected_string = "GET response";
    server.set_get_handler("/v1/license", [&](httplib::Response& resp) {
        resp.set_content(expected_string, "text/plain");
    });

    auto resp = cpr::Get(
        cpr::Url{"http://localhost:" + std::to_string(server.get_port()) +
                 "/v1/license"},
        cpr::Authentication{"test_user", "password", cpr::AuthMode::BASIC});

    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(resp.text == expected_string);
}

BOOST_AUTO_TEST_CASE(handles_post) {
    auto server = http_server("test_user", "password");
    server.set_post_handler(
        "/v1/usage", [](const httplib::Request& req, httplib::Response& resp) {
            resp.set_content("usage_report:" + req.body, "text/plain");
        });

    auto resp = cpr::Post(
        cpr::Url{"http://localhost:" + std::to_string(server.get_port()) +
                 "/v1/usage"},                       //
        cpr::Body{"10gibs"},                         //
        cpr::Header{{"Content-Type", "text/plain"}}, //
        cpr::Authentication{"test_user", "password", cpr::AuthMode::BASIC});

    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(resp.text == "usage_report:10gibs");
}

BOOST_AUTO_TEST_SUITE_END()
