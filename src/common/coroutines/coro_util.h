#pragma once

#include <common/coroutines/promise.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <ranges>

namespace uh::cluster {

template <typename R, typename I>
coro<std::conditional_t<std::is_void_v<R>, void, std::vector<R>>>
run_for_all(boost::asio::io_context& ioc,
            std::function<coro<R>(size_t, I)> func,
            const std::vector<I>& inputs) {
    std::vector<future<R>> futures;
    futures.reserve(inputs.size());

    size_t i = 0;
    auto context = co_await boost::asio::this_coro::context;
    for (const auto& in : inputs) {
        promise<R> p;
        futures.emplace_back(p.get_future());

        boost::asio::co_spawn(ioc, func(i++, in).continue_trace(context),
                              use_promise_cospawn(std::move(p)));
    }

    if constexpr (std::is_void_v<R>) {
        for (auto& f : futures) {
            co_await f.get();
        }
        co_return;
    } else {
        std::vector<R> res;
        res.reserve(inputs.size());
        for (auto& f : futures) {
            res.emplace_back(co_await f.get());
        }
        co_return res;
    }
}

template <typename R, typename K, typename V, typename Func>
coro<std::conditional_t<std::is_void_v<R>, void, std::unordered_map<K, R>>>
run_for_all(boost::asio::io_context& ioc, Func func,
            const std::unordered_map<K, V>& inputs) {
    std::vector<std::pair<K, future<R>>> futures;
    futures.reserve(inputs.size());

    auto context = co_await boost::asio::this_coro::context;
    for (const auto& [k, v] : inputs) {
        promise<R> p;
        futures.emplace_back(k, p.get_future());

        boost::asio::co_spawn(ioc, func(k, v).continue_trace(context),
                              use_promise_cospawn(std::move(p)));
    }

    if constexpr (std::is_void_v<R>) {
        for (auto& [_, f] : futures) {
            co_await f.get();
        }
        co_return;
    } else {
        std::unordered_map<K, R> res;
        res.reserve(inputs.size());
        for (auto& [k, f] : futures) {
            res[k] = co_await f.get();
        }
        co_return res;
    }
}

inline auto make_logging_completion_notifier(
    std::string name, std::promise<void>* p = nullptr,
    std::function<void(std::exception_ptr)> on_finish = nullptr) {
    return [name, p, on_finish = std::move(on_finish)](std::exception_ptr e) {
        if (e) {
            try {
                std::rethrow_exception(e);
            } catch (const boost::system::system_error& ex) {
                if (ex.code() == boost::asio::error::operation_aborted) {
                    LOG_INFO() << "[" << name << "] Task cancelled";
                } else if (ex.code() == boost::asio::error::eof or
                           ex.code() == boost::asio::error::bad_descriptor) {
                    LOG_INFO() << "[" << name << "] Disconnected ";
                } else {
                    LOG_WARN() << "[" << name << "] Exception with error code "
                               << ex.code() << " : " << ex.what();
                }
            } catch (const std::exception& ex) {
                LOG_WARN() << "[" << name << "] Exception: " << ex.what();
            } catch (...) {
                LOG_WARN() << "[" << name << "] Unknown non-std exception";
            }
        } else {
            LOG_INFO() << "[" << name << "] Task finished";
        }
        if (on_finish)
            on_finish(e);
        if (p)
            p->set_value();
    };
}

class coro_task {
public:
    template <typename Executor, typename Awaitable>
    coro_task(std::string name, Executor& ioc, Awaitable&& awaitable,
              std::function<void(std::exception_ptr)> on_finish = nullptr)
        : m_name(std::move(name)),
          m_promise{},
          m_future(m_promise.get_future()) {
        boost::asio::co_spawn(
            ioc, std::forward<Awaitable>(awaitable),
            boost::asio::bind_cancellation_slot(
                m_signal.slot(),
                make_logging_completion_notifier(m_name, &m_promise,
                                                 std::move(on_finish))));
    }

    coro_task(const coro_task&) = delete;
    coro_task& operator=(const coro_task&) = delete;
    coro_task(coro_task&&) = delete;
    coro_task& operator=(coro_task&&) = delete;

    ~coro_task() {
        cancel();
        wait();
    }

    void cancel() { //
        m_signal.emit(boost::asio::cancellation_type::all);
    }

    void wait() {
        try {
            m_future.get();
        } catch (const std::future_error& e) {
            LOG_ERROR() << "[" << m_name
                        << "] future_error in wait(): " << e.what();
        } catch (const std::exception& e) {
            LOG_ERROR() << "[" << m_name
                        << "] exception in wait(): " << e.what();
        } catch (...) {
            LOG_ERROR() << "[" << m_name << "] unknown exception in wait()";
        }
    }

private:
    std::string m_name;
    std::promise<void> m_promise;
    std::future<void> m_future;
    boost::asio::cancellation_signal m_signal;
};

} // namespace uh::cluster
