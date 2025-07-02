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

    void run() {
        auto workguard = boost::asio::make_work_guard(m_ioc);
        for (unsigned i = 0; i < m_num_threads; ++i)
            m_threads.emplace_back([this] {
                try {
                    m_ioc.run();
                } catch (const std::exception& e) {
                    LOG_FATAL() << "io_context.run() terminated by exception: "
                                << e.what();
                    std::terminate();
                } catch (...) {
                    LOG_FATAL()
                        << "io_context.run() terminated by unknown exception";
                    std::terminate();
                }
            });

        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (m_signal_received) {
                LOG_INFO() << "Signal received before service creation. "
                              "Skipping service creation.";
            } else {
                try {
                    for (auto& factory : m_service_factories)
                        m_svcs.push_back(factory(m_ioc));
                } catch (const std::exception& e) {
                    LOG_FATAL() << "Service creation failed: " << e.what();
                    std::terminate();
                } catch (...) {
                    LOG_FATAL() << "Service creation failed with unknown error";
                    std::terminate();
                }
            }
        }

        workguard.reset();

        for (auto& t : m_threads)
            t.join();
    }

    boost::asio::io_context& get_executor() { return m_ioc; }

private:
    void handle_signal() {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_signal_received = true;
        m_svcs.clear();
        m_ioc.stop();
    }

    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_signals;
    std::vector<std::any> m_svcs;
    std::mutex m_mtx;
    bool m_signal_received = false;

    std::vector<std::thread> m_threads;
    std::vector<std::function<std::any(boost::asio::io_context&)>>
        m_service_factories;
    unsigned m_num_threads;
};

} // namespace uh::cluster
