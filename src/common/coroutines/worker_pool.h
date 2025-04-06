#pragma once

#include "common/coroutines/promise.h"
#include <exception>
#include <memory>

namespace uh::cluster {

class worker_pool {

public:
    worker_pool(size_t worker_count)
        : m_threads(worker_count) {}

    template <typename Func>
    requires(!std::is_void_v<std::invoke_result_t<Func>>)
    coro<std::invoke_result_t<Func>> post_in_workers(Func func) {
        promise<std::invoke_result_t<Func>> p;
        auto fut = p.get_future();

        auto context = co_await boost::asio::this_coro::context;
        auto f = [context](auto& f, auto&& promise) mutable {
            THREAD_LOCAL_CONTEXT = context;

            if (boost::asio::trace_span::enable &&
                !boost::asio::trace_span::check_context(context)) {
                LOG_ERROR() << "[post_in_workers] The context to be "
                               "encoded is invalid";
            }

            try {
                promise.set_value(f());
            } catch (const std::exception&) {
                promise.set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads,
                          std::bind(f, std::ref(func), std::move(p)));

        co_return co_await fut.get();
    }

    template <typename Func>
    requires(std::is_void_v<std::invoke_result_t<Func>>)
    coro<void> post_in_workers(Func func) {
        promise<void> p;
        auto fut = p.get_future();

        auto context = co_await boost::asio::this_coro::context;

        auto f = [context](auto& f, auto&& promise) mutable {
            try {
                THREAD_LOCAL_CONTEXT = context;

                if (boost::asio::trace_span::enable &&
                    !boost::asio::trace_span::check_context(context)) {
                    LOG_ERROR() << "[post_in_workers] The context to be "
                                   "encoded is invalid";
                }

                f();
                promise.set_value();
            } catch (const std::exception&) {
                promise.set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads,
                          std::bind(f, std::ref(func), std::move(p)));

        co_await fut.get();
    }

    ~worker_pool() {
        m_threads.join();
        m_threads.stop();
    }

private:
    boost::asio::thread_pool m_threads;
};

} // end namespace uh::cluster
