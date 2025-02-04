#define BOOST_TEST_MODULE "async_http_client tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/network/async_http_client.h>

#include <boost/beast/http/status.hpp>
#include <lib/util/coroutine.h>
#include <nlohmann/json.hpp>

using nlohmann::json;
using namespace uh::cluster;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()} {}

    io_context& ioc;
};

BOOST_FIXTURE_TEST_SUITE(a_async_http_client, fixture)

BOOST_AUTO_TEST_CASE(can_get_through_http_and_basic_auth_with_using_future) {
    auto sut = async_http_client{"ultihash", "passwd", cpr::AuthMode::BASIC};
    json expected_json = {{"authenticated", true}, {"user", "ultihash"}};

    auto future = sut.async_get(
        "https://www.httpbin.org/basic-auth/ultihash/passwd", use_future);

    auto resp = future.get();
    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(json::parse(resp.text).dump() == expected_json.dump());
}

BOOST_AUTO_TEST_CASE(can_get_through_http_and_basic_auth_with_using_awaitable) {
    auto sut = async_http_client{"ultihash", "passwd", cpr::AuthMode::BASIC};
    json expected_json = {{"authenticated", true}, {"user", "ultihash"}};

    auto future = co_spawn(
        ioc,
        sut.async_get("https://www.httpbin.org/basic-auth/ultihash/passwd",
                      use_awaitable),
        use_future);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto resp = future.get();
    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(json::parse(resp.text).dump() == expected_json.dump());
}

BOOST_AUTO_TEST_CASE(can_get_through_http_and_basic_auth_with_using_callback) {
    std::promise<cpr::Response> promise;
    std::future<cpr::Response> future = promise.get_future();
    auto sut = async_http_client{"ultihash", "passwd", cpr::AuthMode::BASIC};
    json expected_json = {{"authenticated", true}, {"user", "ultihash"}};

    sut.async_get("https://www.httpbin.org/basic-auth/ultihash/passwd",
                  [&](auto resp) { promise.set_value(resp); });

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto resp = future.get();
    BOOST_TEST(resp.status_code ==
               static_cast<int>(boost::beast::http::status::ok));
    BOOST_TEST(json::parse(resp.text).dump() == expected_json.dump());
}

BOOST_AUTO_TEST_SUITE_END()
