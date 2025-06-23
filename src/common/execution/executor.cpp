#include "executor.h"

namespace uh::cluster {

executor::executor(unsigned threads)
    : m_ioc(threads),
      m_threads(threads) {
}

void executor::run() {
    LOG_DEBUG() << "starting executor";

    {
        std::unique_lock lock(m_stop_mutex);
        m_stopped = false;
    }

    std::vector<std::thread> threads;
    for (unsigned i = 0; i < m_threads; i++) {
        threads.emplace_back([this](){ m_ioc.run(); });
    }

    for (auto& t : threads) {
        t.join();
    }

    LOG_DEBUG() << "ending executor";

    {
        std::unique_lock lock(m_stop_mutex);
        m_stopped = true;
    }
    m_stop_cond.notify_all();
}

void executor::stop() {
    m_stop.request_stop();

    std::unique_lock lock(m_mutex);
    for (auto& func : m_stop_functions) {
        func();
    }
}

void executor::wait() {
    std::unique_lock lock(m_stop_mutex);
    if (!m_stopped) {
        m_stop_cond.wait(lock, [this](){ return m_stopped; });
    }
}

void executor::keep_alive() {
    m_work_guard = std::make_unique<boost::asio::executor_work_guard<decltype(m_ioc.get_executor())>>(m_ioc.get_executor());

    {
        std::unique_lock lock(m_mutex);
        m_stop_functions.emplace_back([this](){ m_work_guard.reset(); });
    }
}

}
