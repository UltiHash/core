#ifndef UH_CLUSTER_AWAITABLE_FUTURE_H
#define UH_CLUSTER_AWAITABLE_FUTURE_H

#include <boost/asio/steady_timer.hpp>
#include <type_traits>
#include "common/network/messenger_core.h"

namespace uh::cluster {

template <typename T>
class awaitable_promise {

    boost::asio::steady_timer m_waiter;
    std::optional <T> m_data;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc):
            m_waiter (ioc, boost::asio::steady_timer::clock_type::duration::max ()) {}

    inline void set (T&& data) {
        m_data.emplace (std::move (data));
        m_waiter.cancel();
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    coro <T> get () {
        co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));
        std::atomic_thread_fence(std::memory_order_seq_cst);
        co_return std::move (*m_data);
    }

};

template <>
class awaitable_promise <void> {

    boost::asio::steady_timer m_waiter;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc):
            m_waiter (ioc, boost::asio::steady_timer::clock_type::duration::max ()) {}

    inline void set () {
        m_waiter.cancel();
    }

    coro <void> get () {
        co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));
    }

};
}

#endif //UH_CLUSTER_AWAITABLE_FUTURE_H
