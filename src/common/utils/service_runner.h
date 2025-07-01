#pragma once

#include <common/telemetry/log.h>

#include <any>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace uh::cluster {

class service_runner {
public:
    service_runner(
        std::vector<std::function<std::any(boost::asio::io_context&)>>
            service_factories,
        unsigned num_threads = 1)
        : m_ioc(num_threads),
          m_signals(m_ioc, SIGINT, SIGTERM),
          m_service_factories(std::move(service_factories)),
          m_num_threads(num_threads) {
        m_signals.async_wait(
            [this](const boost::system::error_code&, int) { handle_signal(); });
    }

    service_runner(
        std::function<std::any(boost::asio::io_context&)> service_factory,
        unsigned num_threads = 1)
        : service_runner(std::vector{std::move(service_factory)}, num_threads) {
    }

    bool is_running() const {
        for (const auto& t : m_threads) {
            if (t.joinable())
                return true;
        }
        return false;
    }

    void run() {
        auto workguard = boost::asio::make_work_guard(m_ioc);
        for (unsigned i = 0; i < m_num_threads; ++i)
            m_threads.emplace_back([this] { m_ioc.run(); });

        {
            std::lock_guard lock(m_mtx);
            m_creating = true;
        }
        try {
            for (auto& factory : m_service_factories)
                m_svcs.push_back(factory(m_ioc));
        } catch (const std::exception& e) {
            LOG_ERROR() << "Error in service creation: " << e.what();
        }
        {
            std::lock_guard lock(m_mtx);
            m_creating = false;
            m_cv.notify_all();
        }

        workguard.reset();
        for (auto& t : m_threads)
            t.join();
    }

    boost::asio::io_context& get_executor() { return m_ioc; }

private:
    void handle_signal() {
        std::unique_lock lock(m_mtx);
        if (!m_svcs.empty()) {
            m_svcs.clear();
        } else if (m_creating) {
            LOG_ERROR()
                << "service is already being created, waiting for it to finish";
            m_cv.wait(lock, [&] { return !m_creating; });
            m_svcs.clear();
        }
        m_ioc.stop();
    }

    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_signals;
    std::vector<std::any> m_svcs;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_creating = false;
    std::vector<std::thread> m_threads;
    std::vector<std::function<std::any(boost::asio::io_context&)>>
        m_service_factories;
    unsigned m_num_threads;
};

} // namespace uh::cluster
