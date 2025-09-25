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
using namespace boost::asio::experimental;

template <typename Func, typename CompletionToken>
auto wrap_coro_as_deferred(io_context& ioc, Func&& func,
                           CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken,
                                       void(boost::system::error_code)>(
        boost::asio::co_composed<void(boost::system::error_code)>(
            [func = std::forward<Func>(func)](auto state,
                                              io_context& ioc) -> void {
                try {
                    state.throw_if_cancelled(true);
                    state.reset_cancellation_state(
                        boost::asio::enable_terminal_cancellation());
                    // Call the provided function and await its result
                    co_await func();
                } catch (const boost::system::system_error& e) {
                    co_return {e.code()};
                }
            },
            ioc),
        token, std::ref(ioc));
}

BOOST_FIXTURE_TEST_SUITE(a_traced_coro, fixture)
BOOST_AUTO_TEST_CASE(ranged_parallel_group_streambuf) {

    boost::asio::posix::stream_descriptor out(ioc, ::dup(STDOUT_FILENO));
    boost::asio::posix::stream_descriptor err(ioc, ::dup(STDERR_FILENO));

    using op_type = decltype(out.async_write_some(boost::asio::buffer("", 0)));
    std::cout << boost::core::demangle(typeid(op_type).name()) << "\n";
    auto tmp = wrap_coro_as_deferred(
        ioc,
        []() -> coro<void> {
            // Your coroutine code here
            co_return;
        },
        deferred);
    using op_type_2 =
        decltype(out.async_write_some(boost::asio::buffer("", 0)));
    std::cout << boost::core::demangle(typeid(op_type_2).name()) << "\n";

    // std::vector<op_type> ops;
    //
    // ops.push_back(out.async_write_some(boost::asio::buffer("first\r\n", 7),
    //                                    use_awaitable));
    //
    // ops.push_back(err.async_write_some(boost::asio::buffer("second\r\n", 8),
    //                                    use_awaitable));
    //
    // co_spawn(
    //     ioc,
    //     [&]() -> coro<void> {
    //         auto [completion_order, n] =
    //             co_await make_parallel_group(ops).async_wait(
    //                 wait_for_one_error(), deferred);
    //
    //         for (std::size_t i = 0; i < completion_order.size(); ++i) {
    //             std::size_t idx = completion_order[i];
    //             std::cout << "operation " << idx << " finished: ";
    //             std::cout << n[idx] << "\n";
    //         }
    //     },
    //     use_future)
    //     .get();
}
BOOST_AUTO_TEST_SUITE_END()
