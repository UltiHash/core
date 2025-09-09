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

namespace impl {
inline auto make_logging_completion_notifier(
    const std::string& name, std::promise<void>& promise,
    std::function<void(std::exception_ptr)> on_finish = nullptr) {

    return [&name, &promise,
            on_finish = std::move(on_finish)](std::exception_ptr e) {
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
            on_finish(e);

        LOG_INFO() << "[" << name << "] completion handler: set promise ";
        promise.set_value();
        // NOTE: You cannot use `name` and `promise` after this point
    };
}
} // namespace impl

class task {
public:
    task(std::string name, boost::asio::io_context& ioc)
        : m_name{std::move(name)},
          m_strand(boost::asio::make_strand(ioc)),
          m_promise{},
          m_future(m_promise.get_future()) {}

    template <typename T>
    void spawn(T&& t,
               std::function<void(std::exception_ptr)> on_finish = nullptr) {
        m_spawned = true;
        boost::asio::co_spawn(
            m_strand, std::forward<T>(t),
            boost::asio::bind_cancellation_slot(
                m_signal.slot(), impl::make_logging_completion_notifier(
                                     m_name, m_promise, on_finish)));
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&&) = delete;
    task& operator=(task&&) = delete;

    ~task() {
        if (m_spawned) {
            wait();
        }
    }

    void cancel() {
        std::promise<void> promise;
        auto future = promise.get_future();

        // use dispatch, since destroyer can be called from the strand
        boost::asio::dispatch(m_strand, [this, &promise]() {
            m_signal.emit(boost::asio::cancellation_type::all);
            promise.set_value();
        });
        future.get();
    }

private:
    std::string m_name;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::promise<void> m_promise;
    std::future<void> m_future;
    boost::asio::cancellation_signal m_signal;
    bool m_spawned{false};

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
};

class scoped_task : public task {
public:
    template <typename T>
    scoped_task(std::string name, boost::asio::io_context& ioc, T&& t)
        : task(name, ioc) {
        task::spawn(std::forward<T>(t));
    }

    scoped_task(const scoped_task&) = delete;
    scoped_task& operator=(const scoped_task&) = delete;
    scoped_task(scoped_task&&) = delete;
    scoped_task& operator=(scoped_task&&) = delete;

    ~scoped_task() { task::cancel(); }
};

} // namespace uh::cluster
