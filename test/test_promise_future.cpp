#define BOOST_TEST_MODULE "awaitable_promise tests"

#include "common/coroutines/promise.h"
#include <boost/test/unit_test.hpp>

using namespace boost::asio;

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

class coro_fixture {
public:
    coro_fixture()
        : m_ctx(thread_count),
          m_work_guard(m_ctx.get_executor()) {
        for (std::size_t i = 0ull; i < thread_count; ++i) {
            m_threads.push_back(std::thread([this]() { m_ctx.run(); }));
        }
    }

    auto spawn(auto& func) {
        return co_spawn(m_ctx, func(), boost::asio::use_future);
    }

    ~coro_fixture() {
        m_work_guard.reset();
        m_ctx.stop();

        for (auto& t : m_threads) {
            t.join();
        }
    }

    static constexpr std::size_t thread_count = 2;

private:
    io_context m_ctx;
    executor_work_guard<decltype(m_ctx.get_executor())> m_work_guard;
    std::list<std::thread> m_threads;
};

BOOST_FIXTURE_TEST_CASE(future_get, coro_fixture) {
    {
        uh::cluster::promise<int> p;

        auto f = [&p]() -> coro<int> {
            auto f = p.get_future();
            co_return co_await f.get();
        };
        std::future<int> res = spawn(f);

        p.set_value(4711);
        BOOST_CHECK(res.get() == 4711);
    }

    {
        uh::cluster::promise<int> p;
        p.set_value(4711);

        auto f = [&]() -> coro<int> {
            auto f = p.get_future();
            co_return co_await f.get();
        };
        std::future<int> res = spawn(f);

        BOOST_CHECK(res.get() == 4711);
    }
}

BOOST_AUTO_TEST_CASE(basic) {
    {
        std::future<int> f;
        BOOST_CHECK(!f.valid());
    }

    {
        std::promise<int> p;
        std::future<int> f = p.get_future();
        BOOST_CHECK(f.valid());
    }
}

BOOST_FIXTURE_TEST_CASE(pass_exception, coro_fixture) {
    uh::cluster::promise<int> p;

    auto f = [&]() -> coro<int> {
        auto f = p.get_future();
        co_return co_await f.get();
    };
    std::future<int> res = spawn(f);

    try {
        throw 1;
    } catch (...) {
        p.set_exception(std::current_exception());
    }

    BOOST_CHECK_THROW(res.get(), int);
}

BOOST_FIXTURE_TEST_CASE(errors, coro_fixture) {
    {
        std::promise<int> p;
        std::promise<int> p2 = std::move(p);
        BOOST_CHECK_THROW(p.get_future(), std::future_error);
    }
    {
        std::promise<int> p;
        auto f = p.get_future();
        BOOST_CHECK_THROW(p.get_future(), std::future_error);
    }
    {
        std::promise<int> p;
        std::promise<int> p2 = std::move(p);
        BOOST_CHECK_THROW(p.set_value(1), std::future_error);
    }
    {
        std::promise<int> p;
        p.set_value(1);
        BOOST_CHECK_THROW(p.set_value(2), std::future_error);
    }
    {
        std::promise<int> p;
        try {
            throw 1;
        } catch (...) {
            p.set_exception(std::current_exception());
        }
        BOOST_CHECK_THROW(p.set_value(1), std::future_error);
    }
    {
        std::promise<int> p;
        p.set_value(1);
        try {
            throw 1;
        } catch (...) {
            BOOST_CHECK_THROW(p.set_exception(std::current_exception()),
                              std::future_error);
        }
    }
}

} // namespace uh::cluster
