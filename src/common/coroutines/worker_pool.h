#ifndef UH_CLUSTER_WORKER_POOL_H
#define UH_CLUSTER_WORKER_POOL_H

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
    coro<std::invoke_result_t<Func>> post_in_workers(context& ctx, Func func) {
        promise<std::invoke_result_t<Func>> p;
        auto fut = p.get_future();

        auto f = [ctx](auto& f, auto&& promise) mutable {
            CURRENT_CONTEXT = ctx;
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
    coro<void> post_in_workers(context& ctx, Func func) {
        promise<void> p;
        auto fut = p.get_future();

        auto f = [ctx](auto& f, auto&& promise) mutable {
            try {
                CURRENT_CONTEXT = ctx;
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
#endif // UH_CLUSTER_WORKER_POOL_H
