#define BOOST_TEST_MODULE "async_http_client tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/network/async_http_client.h>

#include <lib/mock/http_server/http_server.h>
#include <lib/util/coroutine.h>
#include <nlohmann/json.hpp>

using nlohmann::json;
using namespace uh::cluster;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()},
          server("ultihash", "passwd"),
          expected_string("sample_license"),
          sut("ultihash", "passwd", cpr::AuthMode::BASIC) {
        server.set_get_handler("/v1/license", [&](httplib::Response& resp) {
            resp.set_content(expected_string, "text/plain");
        });
    }

    io_context& ioc;
    http_server server;
    std::string expected_string;
    async_http_client sut;
};

BOOST_FIXTURE_TEST_SUITE(a_async_http_client, fixture)

BOOST_AUTO_TEST_CASE(can_get_through_http_and_basic_auth_with_using_future) {
    auto future = sut.async_get(
        "http://localhost:" + std::to_string(server.get_port()) + "/v1/license",
        use_future);

    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto resp = future.get();
    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(resp.text == expected_string);
}

BOOST_AUTO_TEST_CASE(can_get_through_http_and_basic_auth_with_using_awaitable) {
    auto future = co_spawn(
        ioc,
        sut.async_get("http://localhost:" + std::to_string(server.get_port()) +
                          "/v1/license",
                      use_awaitable),
        use_future);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto resp = future.get();
    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(resp.text == expected_string);
}

BOOST_AUTO_TEST_CASE(can_get_through_http_and_basic_auth_with_using_callback) {
    std::promise<cpr::Response> promise;
    std::future<cpr::Response> future = promise.get_future();

    sut.async_get("http://localhost:" + std::to_string(server.get_port()) +
                      "/v1/license",
                  [&](auto resp) { promise.set_value(resp); });

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto resp = future.get();
    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(resp.text == expected_string);
}

BOOST_AUTO_TEST_SUITE_END()
