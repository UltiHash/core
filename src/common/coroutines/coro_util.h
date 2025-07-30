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

class coro_task;

inline auto make_logging_completion_notifier(
    const std::string& name, std::promise<void>& promise,
    std::shared_ptr<coro_task> self,
    std::function<void(std::shared_ptr<coro_task>, std::exception_ptr)>
        on_finish = nullptr) {

    return [self, &name, &promise,
            on_finish = std::move(on_finish)](std::exception_ptr e) {
        LOG_INFO() << "[" << name << "] completion handler";
        if (e) {
            try {
                std::rethrow_exception(e);
            } catch (const boost::system::system_error& ex) {
                if (ex.code() == boost::asio::error::operation_aborted) {
                    LOG_INFO() << "[" << name
                               << "] completion handler: task cancelled";
                } else if (ex.code() == boost::asio::error::eof or
                           ex.code() == boost::asio::error::bad_descriptor) {
                    LOG_INFO()
                        << "[" << name << "] completion handler: disconnected ";
                } else {
                    LOG_WARN() << "[" << name
                               << "] completion handler: exception with "
                                  "error code "
                               << ex.code() << " : " << ex.what();
                }
            } catch (const std::exception& ex) {
                LOG_WARN() << "[" << name << "] completion handler: exception, "
                           << ex.what();
            } catch (...) {
                LOG_WARN() << "[" << name
                           << "] completion handler: unknown non-std exception";
            }
        }

        if (on_finish)
            on_finish(self, e);

        LOG_INFO() << "[" << name << "] completion handler: set promise ";
        promise.set_value();
    };
}

class coro_task : public std::enable_shared_from_this<coro_task> {

public:
    static auto create(std::string name, boost::asio::io_context& ioc) {
        return std::make_shared<coro_task>(std::move(name), ioc);
    }

    coro_task(std::string name, boost::asio::io_context& ioc)
        : m_name{std::move(name)},
          m_strand(boost::asio::make_strand(ioc)),
          m_promise{},
          m_future(m_promise.get_future()) {}

public:
    template <typename T>
    void
    spawn(boost::asio::traced_awaitable<T>&& aw,
          std::function<void(std::shared_ptr<coro_task>, std::exception_ptr)>
              on_finish = nullptr) {
        LOG_DEBUG() << "[" << m_name
                    << "] About to call shared_from_this() in spawn()";
        auto self = shared_from_this();
        LOG_DEBUG() << "[" << m_name
                    << "] shared_from_this() succeeded in spawn()";
        boost::asio::co_spawn(
            m_strand,
            [aw = std::move(aw)]() mutable -> boost::asio::awaitable<void> {
                co_await boost::asio::this_coro::reset_cancellation_state(
                    boost::asio::enable_terminal_cancellation());
                co_await std::move(aw);
            },
            boost::asio::bind_cancellation_slot(
                m_signal.slot(),
                // [self](std::exception_ptr _) { self->m_promise.set_value();
                // }));
                make_logging_completion_notifier(m_name, m_promise, self,
                                                 on_finish)));
    }

    template <typename Function,
              std::enable_if_t<std::is_invocable_v<Function>, int> = 0>
    void
    spawn(Function&& f,
          std::function<void(std::shared_ptr<coro_task>, std::exception_ptr)>
              on_finish = nullptr) {
        LOG_DEBUG() << "[" << m_name
                    << "] About to call shared_from_this() in spawn()";
        auto self = shared_from_this();
        LOG_DEBUG() << "[" << m_name
                    << "] shared_from_this() succeeded in spawn()";
        boost::asio::co_spawn(
            m_strand,
            [f = std::forward<Function>(
                 f)]() mutable -> boost::asio::awaitable<void> {
                co_await boost::asio::this_coro::reset_cancellation_state(
                    boost::asio::enable_terminal_cancellation());
                co_await f();
            },
            boost::asio::bind_cancellation_slot(
                m_signal.slot(),
                // [self](std::exception_ptr _) { self->m_promise.set_value();
                // }));
                make_logging_completion_notifier(m_name, m_promise, self,
                                                 on_finish)));
    }
    coro_task(const coro_task&) = delete;
    coro_task& operator=(const coro_task&) = delete;
    coro_task(coro_task&&) = delete;
    coro_task& operator=(coro_task&&) = delete;

    ~coro_task() {
        cancel();
        wait();
    }

    void cancel() {
        LOG_DEBUG() << "[" << m_name
                    << "] About to call shared_from_this() in cancel()";
        std::weak_ptr<coro_task> weak_self = weak_from_this();
        LOG_DEBUG() << "[" << m_name
                    << "] shared_from_this() succeeded in cancel()";
        // Since the posted task can be executed after the object is destroyed,
        boost::asio::post(m_strand, [weak_self, name = m_name]() {
            if (auto self = weak_self.lock()) {
                LOG_DEBUG() << "[" << name << "] emit signal";
                self->m_signal.emit(boost::asio::cancellation_type::all);
            } else {
                LOG_DEBUG() << "[" << name << "] object no longer exists";
            }
        });
    }

    void wait(std::optional<std::chrono::steady_clock::duration> timeout =
                  std::nullopt) {
        try {
            if (timeout.has_value())
                m_future.wait_for(*timeout);
            else
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
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::promise<void> m_promise;
    std::future<void> m_future;
    boost::asio::cancellation_signal m_signal;
};

} // namespace uh::cluster
