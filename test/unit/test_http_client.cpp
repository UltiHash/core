#define BOOST_TEST_MODULE "async_http_client tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/network/http_client.h>

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

BOOST_FIXTURE_TEST_SUITE(a_http_client, fixture)

BOOST_AUTO_TEST_CASE(can_get_response) {
    auto sut =
        uh::cluster::http_client{"ultihash", "passwd", cpr::AuthMode::BASIC};
    json expected_json = {{"authenticated", true}, {"user", "ultihash"}};

    auto future = boost::asio::co_spawn(
        ioc, sut.co_get("https://www.httpbin.org/basic-auth/ultihash/passwd"),
        boost::asio::use_future);

    std::string read_text;
    BOOST_CHECK_NO_THROW(read_text = future.get());
    BOOST_TEST(json::parse(read_text).dump() == expected_json.dump());
}

BOOST_AUTO_TEST_CASE(returns_not_found_for_get_with_invalid_path) {
    auto sut =
        uh::cluster::http_client{"ultihash", "passwd", cpr::AuthMode::BASIC};
    json expected_json = {
        {"data", "The quick brown fox jumps over the lazy dog"}};

    auto future = boost::asio::co_spawn(
        ioc, sut.co_get("https://www.httpbin.org/wrong_path"),
        boost::asio::use_future);

    std::string read_text;
    BOOST_CHECK_THROW(read_text = future.get(), std::system_error);
}

BOOST_AUTO_TEST_SUITE_END()
