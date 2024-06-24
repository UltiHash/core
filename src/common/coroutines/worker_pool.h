#ifndef UH_CLUSTER_WORKER_POOL_H
#define UH_CLUSTER_WORKER_POOL_H

#include "common/coroutines/awaitable_promise.h"
#include "common/network/client.h"
#include "common/network/messenger_core.h"
#include "common/utils/common.h"
#include <boost/asio/steady_timer.hpp>
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
    coro<std::invoke_result_t<Func>> post_in_workers(Func func) {
        auto pr =
            std::make_shared<awaitable_promise<std::invoke_result_t<Func>>>(
                m_ioc);

        auto f = [](auto& f, auto promise) {
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
    coro<void> post_in_workers(Func func) {
        auto pr = std::make_shared<awaitable_promise<void>>(m_ioc);

        auto f = [](auto& f, auto promise) {
            try {
                f();
                promise->set();
            } catch (const std::exception&) {
                promise->set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads, std::bind(f, std::ref(func), pr));

        co_await pr->get();
    }

    template <typename Func, typename In,
              typename R = std::invoke_result_t<Func, In>>
    coro<std::vector<R>>
    broadcast_from_io_thread_in_workers(Func func,
                                        const std::vector<In>& inputs) {
        std::vector<R> results(inputs.size());

        std::vector<std::shared_ptr<awaitable_promise<void>>> promises(
            inputs.size());
        for (auto& pr : promises)
            pr = std::make_shared<awaitable_promise<void>>(m_ioc);

        size_t i = 0;
        for (const auto& in : inputs) {
            auto f = [&results, &in, size = inputs.size(), &promises, &func,
                      i]() {
                try {
                    results[i] = func(in);
                    promises[i]->set();
                } catch (const std::exception&) {
                    promises[i]->set_exception(std::current_exception());
                }
            };

            boost::asio::post(m_threads, f);
            i++;
        }

        std::exception_ptr eptr;
        for (auto& pr : promises) {
            try {
                co_await pr->get();
            } catch (const std::exception& e) {
                LOG_ERROR() << e.what();
                eptr = std::current_exception();
            }
        }

        if (eptr) {
            std::rethrow_exception(eptr);
        }

        co_return results;
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