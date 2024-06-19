#define BOOST_TEST_MODULE "awaitable_promise tests"

#include "common/coroutines/awaitable_promise.h"
#include "common/network/client.h"
#include "common/network/server.h"
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

struct execution_counter {
    void increment() {
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

struct moc_handler : public protocol_handler {
    boost::asio::io_context& m_ioc;
    execution_counter& m_exec_counter;
    explicit moc_handler(boost::asio::io_context& ioc, execution_counter& ex)
        : m_ioc(ioc),
          m_exec_counter(ex) {}
    coro<void> handle(boost::asio::ip::tcp::socket m) override {
        messenger mes(std::move(m));

        for (int i = 0; i < 1000000; i++) {
            auto f = std::make_shared<awaitable_promise<void>>(m_ioc);
            f->set();
            co_await f->get();
        }
        m_exec_counter.increment();
    }
};

BOOST_AUTO_TEST_CASE(dedupe_test) {

    int thread_count = 4;
    uint16_t port = 8088;
    int connections = 4;

    boost::asio::io_context ioc_handler(thread_count);
    boost::asio::io_context ioc_sender(thread_count);
    execution_counter exec_counter;
    server_config config{.threads = static_cast<size_t>(thread_count),
                         .port = port,
                         .bind_address = "0.0.0.0"};

    server s(config, std::make_unique<moc_handler>(ioc_handler, exec_counter),
             ioc_handler);
    std::thread server_thread([&s] { s.run(); });

    client cl(ioc_sender, config.bind_address, config.port, connections);

    exec_counter.wait(connections);
    s.stop();
    server_thread.join();
}

} // namespace uh::cluster
