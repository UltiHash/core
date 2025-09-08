#define BOOST_TEST_MODULE "parallel group tests"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include <chrono>
#include <tuple>

#include "test_config.h"

using boost::asio::awaitable;
using boost::asio::use_awaitable;

using namespace boost::asio::experimental::awaitable_operators;
using namespace std::chrono_literals;

template <typename... Bools> auto logical_and(Bools... bools) {
    return (bools && ...);
}

BOOST_AUTO_TEST_SUITE(a_logical_and)

BOOST_AUTO_TEST_CASE(supports_variable_number_of_operations) {
    BOOST_CHECK(logical_and(true, true, true) == true);
    BOOST_CHECK(logical_and(true, false, true) == false);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(awaitable_and_operator)

template <typename Rep, typename Period>
awaitable<bool> wait_for(boost::asio::io_context& ctx,
                         std::chrono::duration<Rep, Period> duration) {
    boost::asio::steady_timer timer(ctx, duration);
    co_await timer.async_wait(use_awaitable);
    co_return duration < 100ms;
}

template <typename... Delays>
auto create_awaitables(boost::asio::io_context& ctx, Delays... delays) {
    return std::make_tuple(wait_for(ctx, delays)...);
}

template <typename... Ts> auto flatten_tuple(const std::tuple<Ts...>& t);

template <typename T> auto flatten_tuple(const T& t) {
    return std::make_tuple(t);
}

template <typename... Ts> auto flatten_tuple(const std::tuple<Ts...>& t) {
    return std::apply(
        [](auto&&... elems) { return std::tuple_cat(flatten_tuple(elems)...); },
        t);
}

BOOST_AUTO_TEST_CASE(supports_variable_number_of_operations) {
    boost::asio::io_context ctx;
    auto task = [&]() -> awaitable<void> {
        auto awaitables = create_awaitables(ctx, 10us, 10ms, 500ms);
        static_assert(
            std::is_same_v<
                decltype(awaitables),
                std::tuple<awaitable<bool>, awaitable<bool>, awaitable<bool>> //
                >                                                             //
        );
        auto combined =
            std::apply([](auto&&... aws) { return (std::move(aws) && ...); },
                       std::move(awaitables));
        auto result = flatten_tuple(co_await std::move(combined));
        BOOST_CHECK(std::get<0>(result) == true);
        BOOST_CHECK(std::get<1>(result) == true);
        BOOST_CHECK(std::get<2>(result) == false);
    };
    boost::asio::co_spawn(ctx, std::move(task), boost::asio::detached);
    ctx.run_for(1s);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_parallel_group)

BOOST_AUTO_TEST_CASE(supports_variable_number_of_operations) {
    boost::asio::io_context ctx;

    boost::asio::posix::stream_descriptor out(ctx, ::dup(STDOUT_FILENO));
    boost::asio::posix::stream_descriptor err(ctx, ::dup(STDERR_FILENO));

    using op_type = decltype(out.async_write_some(boost::asio::buffer("", 0),
                                                  boost::asio::deferred));

    std::vector<op_type> ops;

    ops.push_back(out.async_write_some(boost::asio::buffer("first\r\n", 7),
                                       boost::asio::deferred));

    ops.push_back(err.async_write_some(boost::asio::buffer("second\r\n", 8),
                                       boost::asio::deferred));

    boost::asio::experimental::make_parallel_group(ops).async_wait(
        boost::asio::experimental::wait_for_all(),
        [](std::vector<std::size_t> completion_order,
           std::vector<boost::system::error_code> ec,
           std::vector<std::size_t> n) {
            BOOST_CHECK(completion_order.size() == 2);
            for (std::size_t i = 0; i < completion_order.size(); ++i) {
                if (auto idx = completion_order[i]; idx == 0)
                    BOOST_CHECK(n[idx] == 7);
                else
                    BOOST_CHECK(n[idx] == 8);
            }
        });

    ctx.run_for(1s);
}

BOOST_AUTO_TEST_SUITE_END()
