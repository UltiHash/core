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

#include <boost/asio/experimental/parallel_group.hpp>

using namespace boost::asio;
using boost::asio::experimental::make_parallel_group;
using boost::asio::experimental::wait_for_one_error;

BOOST_FIXTURE_TEST_SUITE(awaitables, fixture)
awaitable<void> async_timer(io_context& ctx, int ms, std::string& trace) {
    steady_timer timer(ctx, std::chrono::milliseconds(ms));
    co_await timer.async_wait(use_awaitable);
    trace += "done";
    co_return;
}

BOOST_AUTO_TEST_CASE(coroutine_parallel_group_deferred) {

    boost::asio::posix::stream_descriptor out(ioc, ::dup(STDOUT_FILENO));

    using op_type = decltype(out.async_write_some(boost::asio::buffer("", 0)));

    std::cout << boost::core::demangle(typeid(op_type).name()) << std::endl;
    // boost::asio::deferred_async_operation<void (boost::system::error_code,
    // unsigned long),
    // boost::asio::posix::basic_stream_descriptor<boost::asio::any_io_executor>::initiate_async_write_some,
    // boost::asio::const_buffer const&>

    std::string trace1;
    // std::string trace2;
    //
    auto op1 = co_spawn(ioc, async_timer(ioc, 10, trace1), deferred);
    using op_type_2 = decltype(op1);

    std::cout << boost::core::demangle(typeid(op_type_2).name()) << std::endl;
    // boost::asio::deferred_async_operation<void
    // (std::__exception_ptr::exception_ptr),
    // boost::asio::detail::initiate_co_spawn<boost::asio::any_io_executor>,
    // boost::asio::detail::awaitable_as_function<void,
    // boost::asio::any_io_executor> >

    // auto op2 = co_spawn(ioc, async_timer(ioc, 20, trace2), deferred);
    //
    // std::vector<decltype(op1)> ops;
    // ops.push_back(std::move(op1));
    // ops.push_back(std::move(op2));
    //
    // co_spawn(ioc,
    //     [&]() -> awaitable<void> {
    //         auto completion_order =
    //             co_await make_parallel_group(ops)
    //                 .async_wait(wait_for_one_error(), deferred);
    //
    //         BOOST_CHECK_EQUAL(trace1, "done");
    //         BOOST_CHECK_EQUAL(trace2, "done");
    //         co_return;
    //     }(),
    //     use_future
    // ).get();
}

BOOST_AUTO_TEST_SUITE_END()
