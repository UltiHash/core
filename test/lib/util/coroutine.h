#pragma once

#include <boost/asio.hpp>
#include <list>
#include <thread>

namespace uh::cluster {

class coro_fixture {
public:
    coro_fixture(std::size_t thread_count = 2)
        : m_ioc(thread_count),
          m_work_guard(m_ioc.get_executor()) {
        for (std::size_t i = 0ull; i < thread_count; ++i) {
            m_threads.push_back(std::thread([this]() { m_ioc.run(); }));
        }
    }

    template <typename Func,
              typename CompletionToken = decltype(boost::asio::use_future)>
    auto spawn(Func&& func,
               CompletionToken&& token = std::move(boost::asio::use_future)) {
        return co_spawn(m_ioc, std::forward<Func>(func),
                        std::forward<CompletionToken>(token));
    }

    auto& get_io_context() { return m_ioc; }

    ~coro_fixture() {
        m_work_guard.reset();
        m_ioc.stop();

        for (auto& t : m_threads) {
            t.join();
        }
    }

protected:
    boost::asio::io_context m_ioc;
    boost::asio::executor_work_guard<decltype(m_ioc.get_executor())>
        m_work_guard;
    std::list<std::thread> m_threads;
};

} // namespace uh::cluster
