#define BOOST_TEST_MODULE "trace asio tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/telemetry/trace/trace_asio.h>
#include <common/types/common_types.h>

using namespace uh::cluster;

struct fixture {
    fixture()
        : ioc{2},
          work_guard(boost::asio::make_work_guard(ioc)),
          thread{[this] { ioc.run(); }} {
        boost::asio::traced_asio_initialize("test_tracer", "0.1.0");
    }

    ~fixture() { ioc.stop(); }

    boost::asio::io_context ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard;
    std::jthread thread;
};

BOOST_FIXTURE_TEST_SUITE(a_traced_coro, fixture)

BOOST_AUTO_TEST_CASE(set_and_get_value) {

    auto context = opentelemetry::context::Context();
    int64_t val = 42;

    boost::asio::context::set_value(context, "key", val);
    auto ret = boost::asio::context::get_value<int64_t>(context, "key");

    BOOST_TEST(42 == ret);
}

BOOST_AUTO_TEST_CASE(set_and_get_pointer) {

    auto context = opentelemetry::context::Context();
    int val = 42;

    boost::asio::context::set_pointer(context, "key", &val);
    auto p = boost::asio::context::get_pointer<int>(context, "key");

    BOOST_TEST(42 == *p);
}

BOOST_AUTO_TEST_CASE(set_and_get_baggage) {
    auto context = opentelemetry::context::Context();
    auto val = "42";

    boost::asio::context::set_baggage(context, "key", val);
    auto ret = boost::asio::context::get_baggage(context, "key");

    BOOST_TEST("42" == ret);
}

BOOST_AUTO_TEST_CASE(propagates_context_through_coro) {

    boost::asio::co_spawn(
        ioc,
        []() -> coro<void> {
            auto context = co_await boost::asio::this_coro::context;
            boost::asio::context::set_value(context, "peer_port", 11UL);

            co_await [context]() -> coro<void> {
                auto context = co_await boost::asio::this_coro::context;
                BOOST_TEST(11 == boost::asio::context::get_value<uint64_t>(
                                     context, "peer_port"));

                co_await [context]() -> coro<void> {
                    auto context = co_await boost::asio::this_coro::context;
                    BOOST_TEST(11 == boost::asio::context::get_value<uint64_t>(
                                         context, "peer_port"));
                }();
                // clang-format off
            }().continue_trace(context);
        },
        // clang-format on
        boost::asio::use_future)
        .get();
}

BOOST_AUTO_TEST_CASE(propagates_context_through_continue) {

    auto context = opentelemetry::context::Context();
    boost::asio::context::set_value(context, "peer_port", 11UL);

    boost::asio::co_spawn(
        ioc,
        []() -> coro<void> {
            auto context = co_await boost::asio::this_coro::context;
            BOOST_TEST(11 == boost::asio::context::get_value<uint64_t>(
                                 context, "peer_port"));

            co_await []() -> coro<void> {
                auto span = co_await boost::asio::this_coro::span;
                BOOST_TEST(11 == boost::asio::context::get_value<uint64_t>(
                                     span->context(), "peer_port"));
            }();
            // clang-format off
        }().continue_trace(context),
        // clang-format on
        boost::asio::use_future)
        .get();
}

BOOST_AUTO_TEST_SUITE_END()
