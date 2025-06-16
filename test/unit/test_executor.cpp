#define BOOST_TEST_MODULE "executor tests"

#include <boost/test/unit_test.hpp>
#include <common/execution/executor.h>
#include <boost/asio.hpp>

#include "test_config.h"

using namespace uh::cluster;
using namespace std::chrono_literals;

namespace {

coro<void> stop_in(executor& e, std::chrono::milliseconds interval) {
    boost::asio::steady_timer timer(e.get_executor(), interval);
    co_await timer.async_wait();
    e.stop();
}

}

BOOST_AUTO_TEST_CASE(empty) {
    executor e;

    e.run();
}

BOOST_AUTO_TEST_CASE(spawn_method) {
    executor e;

    unsigned value = 0;
    BOOST_CHECK_EQUAL(value, 0);

    auto func = [&value]() -> coro<void> { value = 1; co_return; };
    e.spawn(func);
    e.run();

    BOOST_CHECK_EQUAL(value, 1);
}

BOOST_AUTO_TEST_CASE(spawn_method_forward) {
    executor e;

    unsigned value = 0;
    BOOST_CHECK_EQUAL(value, 0);

    e.spawn([](unsigned& value) -> coro<void> { value = 1; co_return; }, value);
    e.run();

    BOOST_CHECK_EQUAL(value, 1);
}

BOOST_AUTO_TEST_CASE(spawn_class_method_forward) {
    struct helper {
        unsigned& value;
        coro<void> foo() { value = 1; co_return; }
    };

    executor e;

    unsigned value = 0;
    BOOST_CHECK_EQUAL(value, 0);

    helper h{value};
    e.spawn(&helper::foo, &h);
    e.run();

    BOOST_CHECK_EQUAL(value, 1);
}

BOOST_AUTO_TEST_CASE(spawn_class_method_forward_parameter) {
    struct helper {
        coro<void> foo(unsigned& value) { value = 1; co_return; }
    };

    executor e;

    unsigned value = 0;
    BOOST_CHECK_EQUAL(value, 0);

    helper h;
    e.spawn(&helper::foo, &h, value);
    e.run();

    BOOST_CHECK_EQUAL(value, 1);
}

BOOST_AUTO_TEST_CASE(spawn_method_stop_function) {
    unsigned count = 0;

    auto func = [&count](std::stop_token stop) -> coro<void> {
        auto e = co_await boost::asio::this_coro::executor;
        while (!stop.stop_requested()) {
            boost::asio::steady_timer timer(e, 100ms);
            co_await timer.async_wait();
            ++count;
        }
    };

    executor e;
    e.spawn(func);
    e.spawn(stop_in, e, 300ms);
    e.run();

    BOOST_CHECK_GE(count, 2);
    BOOST_CHECK_LT(count, 5);
}

BOOST_AUTO_TEST_CASE(spawn_method_repeated) {
    unsigned count = 0;

    auto func = [&count]() -> coro<void> { ++count; co_return; };

    executor e;
    e.repeated(100ms, func);
    e.spawn(stop_in, e, 300ms);
    e.run();

    BOOST_CHECK_GE(count, 2);
    BOOST_CHECK_LT(count, 5);
}

BOOST_AUTO_TEST_CASE(spawn_method_repeated_no_pause) {
    unsigned count = 0;

    auto func = [&count]() -> coro<void> { ++count; co_return; };

    executor e;
    e.repeated(func);
    e.spawn(stop_in, e, 300ms);
    e.run();

    BOOST_CHECK_GE(count, 0);
}

BOOST_AUTO_TEST_CASE(spawn_method_repeated_stop) {
    std::atomic<bool> stop = false;

    auto func = [&stop]() -> coro<void> {
        auto e = co_await boost::asio::this_coro::executor;
        while (!stop) {
            boost::asio::steady_timer timer(e, 100ms);
            co_await timer.async_wait();
        }
    };

    executor e;
    e.repeated(100ms, [&](){ stop = true; }, func);
    e.spawn(stop_in, e, 300ms);
    e.run();

    BOOST_CHECK(stop);
}
