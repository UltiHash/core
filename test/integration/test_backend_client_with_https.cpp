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
#include <lib/util/coroutine.h>
#include <string>

using namespace fakeit;
using namespace uh::cluster;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()} {}

    io_context& ioc;
};

#include <cpr/cpr.h>

BOOST_FIXTURE_TEST_SUITE(a_backend_client, fixture)

BOOST_AUTO_TEST_CASE(fetch_response_body_test) {
    cpr::Response r =
        cpr::Get(cpr::Url{"https://www.httpbin.org/basic-auth/user/pass"},
                 cpr::Authentication{"user", "pass", cpr::AuthMode::BASIC});
    std::cout << r.text << std::endl;
    // auto future = co_spawn(ioc,
    //                        fetch_response_body(ioc, "httpbin.org",
    //                                            "/basic-auth", "user",
    //                                            "passwd"),
    //                        use_future);
    // std::string response = future.get();
    //
    // BOOST_CHECK(!response.empty());
}

BOOST_AUTO_TEST_SUITE_END()
