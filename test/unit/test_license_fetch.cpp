#define BOOST_TEST_MODULE "license/fetch tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/license/backend_client.h>
#include <common/types/common_types.h>
#include <common/utils/strings.h>
#include <lib/util/coroutine.h>

#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>
#include <boost/url/parse.hpp>

using namespace uh::cluster;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()} {}

    io_context& ioc;
};

BOOST_FIXTURE_TEST_SUITE(a_fetch, fixture)

BOOST_AUTO_TEST_CASE(works) {
    auto future =
        co_spawn(ioc, fetch_response_body(ioc, "example.com"), use_future);
    std::cout << future.get() << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
