#define BOOST_TEST_MODULE "awaitable_promise tests"

#include "common/coroutines/awaitable_promise.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {
/*
BOOST_AUTO_TEST_CASE(basic_promise) {

    boost::asio::io_context ioc;

    boost::asio::co_spawn(
        ioc,
        [&ioc]() -> coro<void> {
            auto pr = std::make_shared<awaitable_promise<int>>(ioc);
            ioc.post([pr]() { pr->set(1); });
            BOOST_TEST((co_await pr->get()) == 1);
        },
        [](const std::exception_ptr& e) {
            if (e)
                std::rethrow_exception(e);
        });

    ioc.run();
}

BOOST_AUTO_TEST_CASE(promise_exception) {

    boost::asio::io_context ioc;

    boost::asio::co_spawn(
        ioc,
        [&ioc]() -> coro<void> {
            auto pr = std::make_shared<awaitable_promise<int>>(ioc);
            ioc.post([pr]() {
                try {
                    throw std::exception{};
                } catch (const std::exception& e) {
                    pr->set_exception(std::current_exception());
                }
            });
            BOOST_CHECK_THROW((co_await pr->get()), std::exception);
        },
        [](const std::exception_ptr& e) {
            if (e)
                std::rethrow_exception(e);
        });

    ioc.run();
}

BOOST_AUTO_TEST_CASE(stress_test) {

    boost::asio::io_context ioc;

    int thread_count = 1000;
    int task_count = 1000000;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    std::atomic<int> failures = 0;

    for (int i = 0; i < task_count; i++) {
        boost::asio::co_spawn(
            ioc,
            [&ioc, i, &failures]() -> coro<void> {
                auto pr = std::make_shared<awaitable_promise<int>>(ioc);
                ioc.post([pr, i]() { pr->set(int(i)); });
                if ((co_await pr->get()) != i) {
                    failures++;
                }
            },
            [](const std::exception_ptr& e) {
                if (e)
                    std::rethrow_exception(e);
            });
    }

    for (int i = 0; i < thread_count; i++) {
        threads.emplace_back([&ioc]() { ioc.run(); });
    }

    for (auto& t : threads) {
        t.join();
    }

    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(stress_test_asio_thread_pool) {

    boost::asio::io_context ioc;

    int thread_count = 1000;
    int task_count = 1000000;

    boost::asio::thread_pool workers(thread_count);
    std::vector<std::thread> io_threads;
    io_threads.reserve(thread_count);

    std::atomic<int> failures = 0;
    for (int i = 0; i < task_count; i++) {
        boost::asio::co_spawn(
            ioc,
            [&ioc, i, &failures, &workers]() -> coro<void> {
                auto pr = std::make_shared<awaitable_promise<int>>(ioc);
                boost::asio::post(workers, [pr, i]() { pr->set(int(i)); });
                if ((co_await pr->get()) != i) {
                    failures++;
                }
            },
            [](const std::exception_ptr& e) {
                if (e)
                    std::rethrow_exception(e);
            });
    }

    for (int i = 0; i < thread_count; i++) {
        io_threads.emplace_back([&ioc]() { ioc.run(); });
    }

    for (auto& t : io_threads) {
        t.join();
    }

    workers.join();

    BOOST_TEST(failures == 0);
}
*/
coro<void> deduplicate_data(boost::asio::io_context& ioc) {

    for (int i = 0; i < 100000; i++) {
        auto f = std::make_shared<awaitable_promise<void>>(ioc);
        f->set();
        co_await f->get();
    }
}

coro<void> waiter (std::vector<std::shared_ptr<awaitable_promise<void>>>& proms) {

    for (auto& p: proms) {
        co_await p->get();
    }
}

BOOST_AUTO_TEST_CASE(dedupe_test) {


    int thread_count = 4;
    boost::asio::io_context ioc (thread_count);

    std::vector<std::thread> io_threads (thread_count);

    std::vector<std::shared_ptr<awaitable_promise<void>>> proms;
    proms.reserve(200);
    for (int i = 0; i < 200; i++) {
        auto p = std::make_shared<awaitable_promise<void>>(
            ioc);
        boost::asio::co_spawn(ioc, deduplicate_data(ioc), use_awaitable_promise_cospawn(p));
        proms.emplace_back(p);
    }


    for (auto& t: io_threads) {
        t = std::thread ([&]{
            ioc.run();
        });
    }

    boost::asio::co_spawn(ioc, waiter(proms), boost::asio::use_future).get();

    for (auto& t : io_threads) {
        t.join();
    }


}

} // namespace uh::cluster
