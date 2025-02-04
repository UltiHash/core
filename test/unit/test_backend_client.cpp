#define BOOST_TEST_MODULE "backend_client tests"

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/test/unit_test.hpp>
#include <common/license/backend_client.h>
#include <fakeit/fakeit.hpp>
#include <future>
#include <lib/mock/http_server/http_server.h>
#include <lib/util/coroutine.h>
#include <string>

using namespace fakeit;
using namespace uh::cluster;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()},
          server("ultihash", "passwd"),
          expected_license("sample_license") {

        server.set_get_handler("/v1/license", [&](httplib::Response& resp) {
            resp.set_content(expected_license, "text/plain");
        });
        server.set_post_handler("/v1/usage", [&](const httplib::Request& req,
                                                 httplib::Response& resp) {
            resp.set_content(req.body, "text/plain");
        });
    }

    io_context& ioc;
    http_server server;
    std::string expected_license;
};

BOOST_FIXTURE_TEST_SUITE(a_backend_client, fixture)

BOOST_AUTO_TEST_CASE(returns_license) {
    auto sut = default_backend_client{
        "localhost:" + std::to_string(server.get_port()), "ultihash", "passwd",
        default_backend_client::type::http};

    auto future =
        boost::asio::co_spawn(ioc, sut.get_license(), boost::asio::use_future);

    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
        BOOST_FAIL("get_license is not finished in expiring time");
    }
    std::string read_license;
    BOOST_CHECK_NO_THROW(read_license = future.get());
    BOOST_TEST(read_license == expected_license);
}

BOOST_AUTO_TEST_CASE(pushs_usage) {
    auto sut = default_backend_client{
        "localhost:" + std::to_string(server.get_port()), "ultihash", "passwd",
        default_backend_client::type::http};

    auto future = boost::asio::co_spawn(ioc, sut.post_usage("my-usage"),
                                        boost::asio::use_future);

    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
        BOOST_FAIL("get_license is not finished in expiring time");
    }
    std::string read_license;
    BOOST_CHECK_NO_THROW(read_license = future.get());
    BOOST_TEST(read_license == "my-usage");
}

BOOST_AUTO_TEST_SUITE_END()
