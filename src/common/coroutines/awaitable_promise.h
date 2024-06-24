#ifndef UH_CLUSTER_AWAITABLE_PROMISE_H
#define UH_CLUSTER_AWAITABLE_PROMISE_H

#include "common/network/messenger_core.h"
#include <boost/asio/steady_timer.hpp>
#include <boost/bind/bind.hpp>
#include <type_traits>

namespace uh::cluster {

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
        try {
            co_await boost::asio::co_spawn(
                m_strand, m_waiter->async_wait(boost::asio::use_awaitable),
                boost::asio::use_awaitable);
        } catch (const boost::system::system_error& e) {
            if (e.code() != boost::asio::error::operation_aborted) {
                throw e;
            }
        }

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
              m_strand,
              boost::asio::steady_timer::clock_type::duration::max())) {}

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
        try {
            co_await boost::asio::co_spawn(
                m_strand, m_waiter->async_wait(boost::asio::use_awaitable),
                boost::asio::use_awaitable);
        } catch (const boost::system::system_error& e) {
            if (e.code() != boost::asio::error::operation_aborted) {
                throw e;
            }
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);

        if (m_exception) {
            std::rethrow_exception(*m_exception);
        }
    }
};

/**
 * Use as a completion token in cojunction with boost async operations.
 *
 * Usage:
 *    auto promise = std::make_shared<awaitable_promise<std::size_t>>(ioc);
 *    ioc.async_read(..., ..., use_awaitable_promise(promise));
 *    ...
 *    auto result = promise->get();
 */
template <typename result>
requires(!std::is_same_v<result, void>)
auto use_awaitable_promise(std::shared_ptr<awaitable_promise<result>> p) {
    return [p](const boost::system::error_code& e, result r) -> void {
        if (e.failed()) {
            try {
                throw std::runtime_error(e.to_string());
            } catch (const std::exception& e) {
                p->set_exception(std::current_exception());
            }
        } else {
            p->set(std::move(r));
        }
    };
}

template <typename result>
requires std::is_same_v<result, void>
auto use_awaitable_promise(std::shared_ptr<awaitable_promise<result>> p) {
    return [p](const boost::system::error_code& e) -> void {
        if (e.failed()) {
            try {
                throw std::runtime_error(e.to_string());
            } catch (const std::exception& e) {
                p->set_exception(std::current_exception());
            }
        } else {
            p->set();
        }
    };
}

template <typename result>
requires(!std::is_same_v<result, void>)
auto use_awaitable_promise_cospawn(
    std::shared_ptr<awaitable_promise<result>> p) {
    return [p](std::exception_ptr e, result r) -> void {
        if (e) {
            p->set_exception(e);
        } else {
            p->set(std::move(r));
        }
    };
}

template <typename result>
requires(std::is_same_v<result, void>)
auto use_awaitable_promise_cospawn(
    std::shared_ptr<awaitable_promise<result>> p) {
    return [p](std::exception_ptr e) -> void {
        if (e) {
            p->set_exception(e);
        } else {
            p->set();
        }
    };
}

} // namespace uh::cluster

#endif // UH_CLUSTER_AWAITABLE_PROMISE_H
