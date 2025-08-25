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

// template<typename... Args>
// constexpr auto logical_and(Args&&... args)
//     -> decltype((std::forward<Args>(args) && ...))
// {
//     return (std::forward<Args>(args) && ...);
// }
BOOST_AUTO_TEST_SUITE(a_logical_and)

BOOST_AUTO_TEST_CASE(supports_variable_number_of_operations) {
    BOOST_CHECK(logical_and(true, true, true) == true);
    BOOST_CHECK(logical_and(true, false, true) == false);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(awaitable_and_operator)

template <typename Rep, typename Period>
awaitable<void> wait_for(boost::asio::io_context& ctx,
                         std::chrono::duration<Rep, Period> duration) {
    boost::asio::steady_timer timer(ctx, duration);
    co_await timer.async_wait(use_awaitable);
}

template <typename... Delays>
auto create_awaitables(boost::asio::io_context& ctx, Delays... delays) {
    return std::make_tuple(wait_for(ctx, delays)...);
}

BOOST_AUTO_TEST_CASE(supports_variable_number_of_operations) {
    boost::asio::io_context ctx;
    auto awaitables = create_awaitables(ctx, 10us, 10ms, 500ms);

    auto combined =
        std::apply([](auto&&... aws) { return (std::move(aws) && ...); },
                   std::move(awaitables));
    boost::asio::co_spawn(ctx, std::move(combined), boost::asio::detached);
    ctx.run_for(1s);
}

BOOST_AUTO_TEST_SUITE_END()
