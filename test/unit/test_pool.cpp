#define BOOST_TEST_MODULE "generic pool tests"

#include <common/utils/pool.h>

#include <util/checks.h>

#include <boost/test/unit_test.hpp>

using namespace boost::asio;
using namespace uh::cluster;

namespace {

BOOST_AUTO_TEST_CASE(get) {
    io_context ioc;
    executor_work_guard<decltype(ioc.get_executor())> work{ioc.get_executor()};
    std::thread thread([&]() { ioc.run(); });

    int id = 5;
    uh::cluster::pool<int> p(
        ioc, [&]() { return std::make_unique<int>(id++); }, 3);

    co_spawn(
        ioc,
        [&]() -> lambda_coro<void> {
            auto r0 = co_await p.get();
            auto r1 = co_await p.get();
            auto r2 = co_await p.get();

            BOOST_CHECK(*r0 >= 5 && *r0 < 8);
            BOOST_CHECK(*r1 >= 5 && *r1 < 8);
            BOOST_CHECK(*r2 >= 5 && *r2 < 8);

            BOOST_CHECK(*r1 != *r2);
            BOOST_CHECK(*r0 != *r2);
            BOOST_CHECK(*r0 != *r1);
        },
        use_future)
        .get();

    work.reset();
    thread.join();
}

BOOST_AUTO_TEST_CASE(block) {
    io_context ioc;
    executor_work_guard<decltype(ioc.get_executor())> work{ioc.get_executor()};

    std::thread thread([&]() { ioc.run(); });

    int id = 5;
    uh::cluster::pool<int> p(
        ioc, [&]() { return std::make_unique<int>(id++); }, 1);

    std::unique_ptr<uh::cluster::pool<int>::handle> handle;

    co_spawn(
        ioc,
        [&]() -> lambda_coro<void> {
            handle = std::make_unique<uh::cluster::pool<int>::handle>(
                co_await p.get());
        },
        use_future)
        .get();

    std::atomic<int> pos = 0;

    auto future = co_spawn(
        ioc,
        [&]() -> lambda_coro<void> {
            ++pos;
            auto r2 = co_await p.get();
            ++pos;
        },
        use_future);

    WAIT_UNTIL_CHECK(500, pos == 1);

    handle->release();
    WAIT_UNTIL_CHECK(500, pos == 2);

    work.reset();
    future.get();
    thread.join();
}

} // namespace
