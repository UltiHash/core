#define BOOST_TEST_MODULE "async_http_client tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

// #include <common/network/http_client.h>

#include <nlohmann/json.hpp>

using nlohmann::json;

BOOST_AUTO_TEST_SUITE(a_http_client)

BOOST_AUTO_TEST_CASE(can_get_response) {
    // boost::asio::io_context ioc;
    // json expected_json = {{"authenticated", true}, {"user", "ultihash"}};
    // auto sut =
    //     uh::cluster::http_client{"ultihash", "passwd", cpr::AuthMode::BASIC};
    //
    // auto future = boost::asio::co_spawn(
    //     ioc,
    //     sut.co_get("https://www.httpbin.org/basic-auth/ultihash/passwd"),
    //     boost::asio::use_future);
    //
    // // BOOST_CHECK_NO_THROW(read_text = future.get());
    // BOOST_TEST(json::parse(future.get()).dump() == expected_json.dump());
}

BOOST_AUTO_TEST_SUITE_END()
