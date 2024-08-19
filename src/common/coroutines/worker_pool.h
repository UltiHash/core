#ifndef UH_CLUSTER_WORKER_POOL_H
#define UH_CLUSTER_WORKER_POOL_H

#include "common/coroutines/awaitable_promise.h"
#include <exception>
#include <memory>

namespace uh::cluster {

class worker_pool {

public:
    worker_pool(boost::asio::io_context& ioc, size_t worker_count)
        : m_threads(worker_count),
          m_ioc(ioc) {}

    template <typename Func>
    requires(!std::is_void_v<std::invoke_result_t<Func>>)
    coro<std::invoke_result_t<Func>> post_in_workers(context& ctx, Func func) {
        auto pr =
            std::make_shared<awaitable_promise<std::invoke_result_t<Func>>>(
                m_ioc);

        auto f = [ctx](auto& f, auto promise) {
            CURRENT_CONTEXT = ctx;
            try {
                promise->set(f());
            } catch (const std::exception&) {
                promise->set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads, std::bind(f, std::ref(func), pr));

        co_return co_await pr->get();
    }

    template <typename Func>
    requires(std::is_void_v<std::invoke_result_t<Func>>)
    coro<void> post_in_workers(context& ctx, Func func) {
        auto pr = std::make_shared<awaitable_promise<void>>(m_ioc);

        auto f = [ctx](auto& f, auto promise) {
            try {
                CURRENT_CONTEXT = ctx;
                f();
                promise->set();
            } catch (const std::exception&) {
                promise->set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads, std::bind(f, std::ref(func), pr));

        co_await pr->get();
    }

    ~worker_pool() {
        m_threads.join();
        m_threads.stop();
    }

private:
    boost::asio::thread_pool m_threads;
    boost::asio::io_context& m_ioc;
};

} // end namespace uh::cluster
#endif // UH_CLUSTER_WORKER_POOL_H