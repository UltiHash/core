#pragma once

#include <common/types/common_types.h>
#include <common/telemetry/log.h>
#include <boost/asio.hpp>
#include <boost/type_traits/function_traits.hpp>

#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

namespace uh::cluster {

class executor {
public:
    executor(unsigned threads = 1);

    /**
     * Run the executor until all jobs are finished. This spawns the configured
     * number of threads.
     */
    void run();

    /**
     * Issue a stop request to the executor and return immediately. This calls all
     * configured stop functions and sets the stop_source to *stop_requested*.
     */
    void stop();

    /**
     * Wait until the executor has shut down. This returns immediately when executor
     * is not running.
     */
    void wait();

    /**
     * Block executor from exiting on its own. This creates a work_guard that is
     * reset when calling `stop()`
     */
    void keep_alive();

    /**
     * Access underlying executor.
     */
    boost::asio::io_context& get_executor() { return m_ioc; }

    /**
     * Spawn a new co-routine in executor.
     */
    template <typename func, typename ... params>
    requires std::invocable<func, std::stop_token, params...>
    void spawn(func&& f, params && ... p) {
        boost::asio::co_spawn(
            m_ioc,
            std::invoke(std::move(f), m_stop.get_token(), std::forward<params>(p)...).start_trace(),
            boost::asio::detached);
    }

    template <typename func, typename ... args>
    void spawn(func&& f, args && ... a) {
        boost::asio::co_spawn(
            m_ioc,
            std::invoke(std::move(f), std::forward<args>(a)...).start_trace(),
            boost::asio::detached);
    }


    /**
     * Repeatedly call `f` until the executor is stopped, waiting `interval` between calls.
     */
    template <typename func, typename ... args>
    requires std::invocable<func, args...>
    void repeated(std::chrono::milliseconds interval, func f, args&& ... a)  {
        spawn(&executor::run_loop<func, args...>, this, std::move(interval), std::move(f), a...);
    }

    /**
     * Repeatedly call `f` until the executor is stopped.
     */
    template <typename func, typename ... args>
    requires std::invocable<func, args...>
    void repeated(func f, args&& ... a)  {
        repeated(std::chrono::milliseconds(0), std::move(f), a...);
    }

    /**
     * Repeatedly call `f` until the executor is stopped.
     * Additionally, register a stop function `stop` that is called from inside `executor::stop()`
     * to cancel any pending operation in `f`.
     */
    template <typename func, std::invocable<> stop_func, typename ... args>
    requires std::invocable<func, args...>
    void repeated(stop_func stop, func f, args&& ... a) {
        {
            std::unique_lock lock(m_mutex);
            m_stop_functions.push_back(std::move(stop));
        }

        repeated(std::move(f), a...);
    }

    /**
     * Repeatedly call `f` until the executor is stopped, waiting `interval` between calls.
     * Additionally, register a stop function `stop` that is called from inside `executor::stop()`
     * to cancel any pending operation in `f`.
     */
    template <typename func, std::invocable<> stop_func, typename ... args>
    requires std::invocable<func, args...>
    void repeated(std::chrono::milliseconds interval, stop_func stop, func f, args&& ... a) {
        {
            std::unique_lock lock(m_mutex);
            m_stop_functions.push_back(std::move(stop));
        }

        repeated(interval, std::move(f), a...);
    }

private:
    template <typename func, typename ... args>
    coro<void> run_loop(std::chrono::milliseconds interval, func f, args&& ... a) {
        while (!m_stop.stop_requested()) {
            auto start_time = std::chrono::steady_clock::now();

            try {
                co_await std::invoke(f, std::forward<args>(a)...);
            } catch (const std::exception& e) {
                LOG_ERROR() << "failure during repeated: " << e.what();
            }

            auto elapsed_time = std::chrono::steady_clock::now() - start_time;
            auto sleep_duration = interval.count() ? interval - elapsed_time : interval;

            boost::asio::steady_timer timer(m_ioc, sleep_duration);
            co_await timer.async_wait();
        }
    }

    boost::asio::io_context m_ioc;
    unsigned m_threads;

    std::mutex m_mutex;
    std::list<std::function<void()>> m_stop_functions;
    std::stop_source m_stop;

    std::mutex m_stop_mutex;
    bool m_stopped = true;
    std::condition_variable m_stop_cond;

    std::unique_ptr<boost::asio::executor_work_guard<decltype(m_ioc.get_executor())>> m_work_guard;
};

}
