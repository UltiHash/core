#define BOOST_TEST_MODULE "exponential backoff tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/license/backend_client.h>
#include <common/license/exp_backoff.h>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/test/included/unit_test.hpp>
#include <chrono>
#include <fakeit/fakeit.hpp>
#include <lib/util/coroutine.h>

using namespace uh::cluster;
using namespace fakeit;
using namespace boost::asio;

class sync_backend_client {
public:
    virtual ~sync_backend_client() = default;
    virtual std::string get_license() = 0;
};

class fixture : coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()} {}

    io_context& ioc;
    Mock<sync_backend_client> mock;
};

BOOST_FIXTURE_TEST_SUITE(a_exp_backoff, fixture)

BOOST_AUTO_TEST_CASE(returns_after_single_try) {
    exponential_backoff<std::string> backoff{ioc, 7, 1, 2};

    Method(mock, get_license) = "sample_license";

    auto future = co_spawn(ioc, //
                           backoff.run([&]() -> coro<std::string> {
                               co_return mock.get().get_license();
                           }),
                           use_future);

    BOOST_CHECK_EQUAL(future.get(), "sample_license");
    Verify(Method(mock, get_license)).Once();
}

BOOST_AUTO_TEST_CASE(returns_after_number_of_failures) {
    exponential_backoff<std::string> backoff{ioc, 7, 1, 2};

    When(Method(mock, get_license))
        .Throw(std::system_error{std::error_code(429, http_category())},
               std::system_error{std::error_code(500, http_category())},
               std::system_error{std::error_code(509, http_category())})
        .Return("sample_license");

    auto future = co_spawn(ioc, //
                           backoff.run([&]() -> coro<std::string> {
                               co_return mock.get().get_license();
                           }),
                           use_future);

    BOOST_CHECK_EQUAL(future.get(), "sample_license");
    Verify(Method(mock, get_license)).Exactly(4_Times);
}

BOOST_AUTO_TEST_CASE(aborts_after_max_retries) {
    exponential_backoff<std::string> backoff{ioc, 3, 1, 2};

    When(Method(mock, get_license))
        .Throw(std::system_error{std::error_code(429, http_category())},
               std::system_error{std::error_code(500, http_category())},
               std::system_error{std::error_code(509, http_category())})
        .Return("sample_license");

    auto future = co_spawn(ioc, //
                           backoff.run([&]() -> coro<std::string> {
                               co_return mock.get().get_license();
                           }),
                           use_future);

    BOOST_CHECK_THROW(future.get(), std::runtime_error);
    Verify(Method(mock, get_license)).Exactly(3_Times);
}

BOOST_AUTO_TEST_CASE(throws_last_unhandled_exception) {
    exponential_backoff<std::string> backoff{ioc, 7, 1, 2};

    When(Method(mock, get_license))
        .Throw(std::runtime_error{""})
        .Return("sample_license");

    auto future = co_spawn(ioc, //
                           backoff.run([&]() -> coro<std::string> {
                               co_return mock.get().get_license();
                           }),
                           use_future);

    BOOST_CHECK_THROW(future.get(), std::runtime_error);
    Verify(Method(mock, get_license)).Once();
}

BOOST_AUTO_TEST_CASE(aborts_execution_for_unauthorized_error) {
    exponential_backoff<std::string> backoff{ioc, 7, 1, 2};

    When(Method(mock, get_license))
        .Throw(std::system_error{std::error_code(401, http_category())})
        .Return("sample_license");

    auto future = co_spawn(ioc, //
                           backoff.run([&]() -> coro<std::string> {
                               co_return mock.get().get_license();
                           }),
                           use_future);

    BOOST_CHECK_THROW(future.get(), std::runtime_error);
    Verify(Method(mock, get_license)).Once();
}

BOOST_AUTO_TEST_SUITE_END()
