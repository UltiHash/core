#ifndef UH_CLUSTER_AWAITABLE_PROMISE_H
#define UH_CLUSTER_AWAITABLE_PROMISE_H

#include "common/network/messenger_core.h"
#include <boost/asio/steady_timer.hpp>
#include <type_traits>

namespace uh::cluster {

template <typename T> class awaitable_promise {

    boost::asio::steady_timer m_waiter;
    std::optional<T> m_data;
    std::optional<std::exception_ptr> m_exception;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc)
        : m_waiter(ioc,
                   boost::asio::steady_timer::clock_type::duration::max()) {}

    inline void set(T&& data) {
        m_data.emplace(std::move(data));
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m_waiter.cancel();
    }

    inline void set_exception(std::exception_ptr ptr) {
        if (m_data || m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_exception = ptr;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m_waiter.expires_after(std::chrono::seconds(0));
    }

    coro<T> get() {
        co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));
        std::atomic_thread_fence(std::memory_order_seq_cst);

        if (m_exception) {
            std::rethrow_exception(*m_exception);
        }

        co_return std::move(*m_data);
    }
};

template <> class awaitable_promise<void> {

    boost::asio::steady_timer m_waiter;
    std::optional<std::exception_ptr> m_exception;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc)
        : m_waiter(ioc,
                   boost::asio::steady_timer::clock_type::duration::max()) {}

    inline void set() { m_waiter.expires_after(std::chrono::seconds(0)); }

    inline void set_exception(std::exception_ptr ptr) {
        if (m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_exception = ptr;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m_waiter.expires_after(std::chrono::seconds(0));
    }

    coro<void> get() {
        co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));

        if (m_exception) {
            std::rethrow_exception(*m_exception);
        }
    }
};
} // namespace uh::cluster

#endif // UH_CLUSTER_AWAITABLE_PROMISE_H
