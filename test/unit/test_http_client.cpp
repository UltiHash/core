#define BOOST_TEST_MODULE "async_http_client tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/network/http_client.h>

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
          expected_license("sample_license"),
          sut{"ultihash", "passwd", cpr::AuthMode::BASIC} {

        server.set_get_handler("/v1/license", [&](httplib::Response& resp) {
            resp.set_content(expected_license, "text/plain");
        });
        server.set_get_handler("/wrong_path", [&](httplib::Response& resp) {
            resp.status = 404;
            resp.set_content("Wrong path", "text/plain");
        });
    }

    io_context& ioc;
    http_server server;
    std::string expected_license;
    uh::cluster::http_client sut;
};

BOOST_FIXTURE_TEST_SUITE(a_http_client, fixture)

BOOST_AUTO_TEST_CASE(can_get_response) {
    auto future = boost::asio::co_spawn(
        ioc,
        sut.co_get("http://localhost:" + std::to_string(server.get_port()) +
                   "/v1/license"),
        boost::asio::use_future);

    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
        BOOST_FAIL("co_get is not finished in expiring time");
    }
    std::string read_license;
    BOOST_CHECK_NO_THROW(read_license = future.get());
    BOOST_TEST(read_license == expected_license);
}

BOOST_AUTO_TEST_CASE(throws_system_error_for_invalid_path) {
    auto future = boost::asio::co_spawn(
        ioc,
        sut.co_get("http://localhost:" + std::to_string(server.get_port()) +
                   "/wrong_path"),
        boost::asio::use_future);

    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
        BOOST_FAIL("co_get is not finished in expiring time");
    }
    std::string read_text;
    BOOST_CHECK_THROW(read_text = future.get(), std::system_error);
}

BOOST_AUTO_TEST_CASE(throws_runtime_error_for_invalid_host_name) {
    auto future = boost::asio::co_spawn(
        ioc,
        sut.co_get("http://-----host:" + std::to_string(server.get_port())),
        boost::asio::use_future);

    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
        BOOST_FAIL("co_get is not finished in expiring time");
    }
    std::string read_text;
    BOOST_CHECK_THROW(read_text = future.get(), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
