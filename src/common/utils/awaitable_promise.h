#ifndef UH_CLUSTER_AWAITABLE_PROMISE_H
#define UH_CLUSTER_AWAITABLE_PROMISE_H

#include "common/network/messenger_core.h"
#include <boost/asio/steady_timer.hpp>
#include <boost/bind/bind.hpp>
#include <memory>
#include <type_traits>

namespace uh::cluster {

namespace awaitable {

template <typename result> class promise;

template <typename result> class future {
public:
    future() noexcept {}

    future(future&& other) noexcept {}

    future(const future& other) = delete;

    coro<result> get() {
        co_await wait();
        co_return std::move(*m_state->value);
    }

    bool valid() const noexcept { return m_state; }

    coro<void> wait() const {
        if (!m_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        return m_state->wait();
    }

private:
    friend class awaitable::promise<result>;

    struct shared_state {
        shared_state(boost::asio::io_context& ioc)
            : ioc(ioc),
              m_strand(ioc.get_executor()),
              m_waiter(m_strand,
                       boost::asio::steady_timer::clock_type::duration::max()) {
        }

        boost::asio::io_context& ioc;
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        boost::asio::steady_timer m_waiter;

        std::optional<std::exception_ptr> m_exception;
        std::optional<result> value;

        void wake_up() {
            boost::asio::post(m_strand, [this]() {
                m_waiter.expires_after(std::chrono::seconds(0));
            });
        }

        coro<void> wait() {
            co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));
            std::atomic_thread_fence(std::memory_order_seq_cst);

            if (m_exception) {
                std::rethrow_exception(*m_exception);
            }

            co_return;
        }
    };

    future(std::shared_ptr<shared_state> state)
        : m_state(state) {}

    std::shared_ptr<shared_state> m_state;
};

template <typename result> class promise {
public:
    promise(boost::asio::io_context& ioc)
        : m_state(std::make_shared<
                  typename awaitable::future<result>::shared_state>(ioc)) {}

    promise(promise&& other)
        : m_state(std::move(other.m_state)) {}

    promise(const promise& other) = delete;

    future<result> get_future() { return future<result>(m_state); }

    void set_value(const result& value) {
        m_state->value = value;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m_state->wake_up();
    }

    void set_value(result&& value) {
        m_state->value = std::move(value);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m_state->wake_up();
    }

    void set_exception(std::exception_ptr p) {
        m_state->m_exception = p;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m_state->wake_up();
    }

private:
    std::shared_ptr<typename awaitable::future<result>::shared_state> m_state;
};

} // namespace awaitable

template <typename T> class awaitable_promise {

    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::shared_ptr<boost::asio::steady_timer> m_waiter;
    std::optional<T> m_data;
    std::optional<std::exception_ptr> m_exception;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc)
        : m_strand(ioc.get_executor()),
          m_waiter(std::make_shared<boost::asio::steady_timer>(
              m_strand,
              boost::asio::steady_timer::clock_type::duration::max())) {}

    inline void set(T&& data) {
        m_data.emplace(std::move(data));
        std::atomic_thread_fence(std::memory_order_seq_cst);
        boost::asio::post(m_strand, [waiter = m_waiter]() {
            waiter->expires_after(std::chrono::seconds(0));
        });
    }

    inline void set_exception(std::exception_ptr ptr) {
        if (m_data || m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_exception = ptr;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        boost::asio::post(m_strand, [waiter = m_waiter]() {
            waiter->expires_after(std::chrono::seconds(0));
        });
    }

    coro<T> get() {
        co_await m_waiter->async_wait(as_tuple(boost::asio::use_awaitable));
        std::atomic_thread_fence(std::memory_order_seq_cst);

        if (m_exception) {
            std::rethrow_exception(*m_exception);
        }

        co_return std::move(*m_data);
    }
};

template <> class awaitable_promise<void> {

    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::shared_ptr<boost::asio::steady_timer> m_waiter;
    std::optional<std::exception_ptr> m_exception;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc)
        : m_strand(ioc.get_executor()),
          m_waiter(std::make_shared<boost::asio::steady_timer>(
              ioc, boost::asio::steady_timer::clock_type::duration::max())) {}

    inline void set() {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        boost::asio::post(m_strand, [waiter = m_waiter]() {
            waiter->expires_after(std::chrono::seconds(0));
        });
    }

    inline void set_exception(std::exception_ptr ptr) {
        if (m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_exception = ptr;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        boost::asio::post(m_strand, [waiter = m_waiter]() {
            waiter->expires_after(std::chrono::seconds(0));
        });
    }

    coro<void> get() {
        co_await m_waiter->async_wait(as_tuple(boost::asio::use_awaitable));
        std::atomic_thread_fence(std::memory_order_seq_cst);

        if (m_exception) {
            std::rethrow_exception(*m_exception);
        }
    }
};
} // namespace uh::cluster

#endif // UH_CLUSTER_AWAITABLE_PROMISE_H
