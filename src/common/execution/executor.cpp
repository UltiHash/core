#include "executor.h"

namespace uh::cluster {

executor::executor(unsigned num_threads)
    : m_ioc(num_threads),
      m_stopped{false} {
    LOG_DEBUG() << "starting executor";

    keep_alive();
    for (unsigned i = 0; i < num_threads; i++) {
        m_threads.emplace_back([this]() { m_ioc.run(); });
    }
}

void executor::run() {

    for (auto& t : m_threads) {
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
        m_stop_cond.wait(lock, [this]() { return m_stopped; });
    }
}

void executor::keep_alive() {
    m_work_guard = std::make_unique<
        boost::asio::executor_work_guard<decltype(m_ioc.get_executor())>>(
        m_ioc.get_executor());

    {
        std::unique_lock lock(m_mutex);
        m_stop_functions.emplace_back([this]() { m_work_guard.reset(); });
    }
}

} // namespace uh::cluster
