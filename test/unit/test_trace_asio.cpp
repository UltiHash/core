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

BOOST_AUTO_TEST_CASE(propagates_context_through_coro) {

    boost::asio::co_spawn(
        ioc,
        []() -> coro<void> {
            auto context = co_await boost::asio::this_coro::context;

            co_await [context]() -> coro<void> {
                auto context = co_await boost::asio::this_coro::context;
                auto peer_port = context.GetValue("peer_port");
                std::cout << "test" << std::endl;
                BOOST_TEST(true == std::holds_alternative<uint64_t>(peer_port));
                BOOST_TEST(11 == std::get<uint64_t>(peer_port));

                co_await [context]() -> coro<void> {
                    auto context = co_await boost::asio::this_coro::context;
                    auto peer_port = context.GetValue("peer_port");
                    std::cout << "test" << std::endl;
                    BOOST_TEST(true ==
                               std::holds_alternative<uint64_t>(peer_port));
                    BOOST_TEST(11 == std::get<uint64_t>(peer_port));
                }();
                // clang-format off
            }().continue_trace(context.SetValue("peer_port", static_cast<uint64_t>(11)));
        },
        // clang-format on
        boost::asio::use_future)
        .get();
}

BOOST_AUTO_TEST_CASE(propagates_context_through_continue) {

    auto context = opentelemetry::context::Context();

    boost::asio::co_spawn(
        ioc,
        []() -> coro<void> {
            auto context = co_await boost::asio::this_coro::context;
            auto peer_port = context.GetValue("peer_port");
            BOOST_TEST(true == std::holds_alternative<uint64_t>(peer_port));
            BOOST_TEST(11 == std::get<uint64_t>(peer_port));

            co_await []() -> coro<void> {
                auto span = co_await boost::asio::this_coro::span;
                auto peer_port = span->context().GetValue("peer_port");
                BOOST_TEST(true == std::holds_alternative<uint64_t>(peer_port));
                BOOST_TEST(11 == std::get<uint64_t>(peer_port));
            }();
            // clang-format off
        }().continue_trace(context.SetValue("peer_port", static_cast<uint64_t>(11))),
        // clang-format on
        boost::asio::use_future)
        .get();
}

BOOST_AUTO_TEST_SUITE_END()
