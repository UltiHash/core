#define BOOST_TEST_MODULE "awaitable_promise tests"

#define BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS

#include <common/coroutines/promise.h>
#include <common/network/client.h>
#include <common/network/server.h>

#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(basic_promise) {

    boost::asio::io_context ioc;

    boost::asio::co_spawn(
        ioc,
        [&ioc]() -> coro<void> {
            promise<int> p;
            auto f = p.get_future();
            boost::asio::post(
                ioc, [p = std::make_shared<uh::cluster::promise<int>>(
                          std::move(p))]() mutable { p->set_value(1); });
            BOOST_TEST((co_await f.get()) == 1);
        },
        [](const std::exception_ptr& e) {
            if (e) {
                std::rethrow_exception(e);
            }
        });

    ioc.run();
}

BOOST_AUTO_TEST_CASE(promise_exception) {

    boost::asio::io_context ioc;

    boost::asio::co_spawn(
        ioc,
        [&ioc]() -> coro<void> {
            promise<int> p;
            auto f = p.get_future();
            boost::asio::post(
                ioc, [p = std::make_shared<uh::cluster::promise<int>>(
                          std::move(p))]() mutable {
                    try {
                        throw std::exception{};
                    } catch (const std::exception& e) {
                        p->set_exception(std::current_exception());
                    }
                });
            BOOST_CHECK_THROW((co_await f.get()), std::exception);
        },
        [](const std::exception_ptr& e) {
            if (e) {
                std::rethrow_exception(e);
            }
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
                promise<int> p;
                auto f = p.get_future();
                boost::asio::post(
                    ioc, [p = std::make_shared<uh::cluster::promise<int>>(
                              std::move(p)),
                          i]() mutable { p->set_value(i); });
                if ((co_await f.get()) != i) {
                    failures++;
                }
            },
            [](const std::exception_ptr& e) {
                if (e) {
                    std::rethrow_exception(e);
                }
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
            [i, &failures, &workers]() -> coro<void> {
                promise<int> p;
                auto f = p.get_future();
                boost::asio::post(
                    workers, [p = std::make_shared<uh::cluster::promise<int>>(
                                  std::move(p)),
                              i]() mutable { p->set_value(int(i)); });
                if ((co_await f.get()) != i) {
                    failures++;
                }
            },
            [](const std::exception_ptr& e) {
                if (e) {
                    std::rethrow_exception(e);
                }
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

struct execution_counter {
    void finished() {
        m_finished++;
        m_cv.notify_one();
    }
    void wait(size_t count) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this, count] { return m_finished == count; });
    }
    [[nodiscard]] size_t count() const { return m_finished; }

private:
    std::atomic_size_t m_finished = 0;
    std::condition_variable m_cv;
    std::mutex m_mutex;
};

coro<void> handle(auto& counter) {
    for (int i = 0; i < 100000; i++) {
        promise<void> p;
        auto f = p.get_future();
        p.set_value();
        co_await f.get();
    }

    counter.finished();
}

coro<void> do_spawn(auto& ioc, auto& counter, int count) {
    while (count > 0) {
        co_spawn(ioc, handle(counter), [](const std::exception_ptr& e) {});
        --count;
    }

    co_return;
}

BOOST_AUTO_TEST_CASE(strand_test) {
    int thread_count = 8;
    int connections = 4;

    boost::asio::io_context ioc_handler(thread_count);
    execution_counter counter;

    co_spawn(ioc_handler, do_spawn(ioc_handler, counter, connections),
             [](const std::exception_ptr& e) {});

    std::list<std::thread> server_threads;
    for (int i = 0; i < thread_count; ++i) {
        server_threads.emplace_back([&] { ioc_handler.run(); });
    }

    counter.wait(connections);
    for (auto& t : server_threads) {
        t.join();
    }
}

} // namespace uh::cluster
