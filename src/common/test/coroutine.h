#pragma once

#include "common/types/common_types.h" // to make coro type visible for users
#include <boost/asio.hpp>
#include <list>
#include <thread>

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
    boost::asio::io_context m_ctx;
    boost::asio::executor_work_guard<decltype(m_ctx.get_executor())>
        m_work_guard;
    std::list<std::thread> m_threads;
};

} // namespace uh::cluster
