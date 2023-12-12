//
// Created by massi on 12/12/23.
//

#ifndef UH_CLUSTER_AWAITABLE_FUTURE_H
#define UH_CLUSTER_AWAITABLE_FUTURE_H

#include <boost/asio/steady_timer.hpp>
#include <type_traits>
#include "network/messenger_core.h"

namespace uh::cluster {

template <typename T>
class awaitable_promise;

template <typename T>
class awaitable_future {
    boost::asio::io_context& m_ioc;
    boost::asio::steady_timer m_waiter;

    friend awaitable_promise <T>;
    std::enable_if <!std::is_same_v <T, void>, T> data;

public:
    explicit awaitable_future (boost::asio::io_context& ioc):
        m_ioc (ioc),
        m_waiter (m_ioc, boost::asio::steady_timer::clock_type::duration::max ()) {
    }

    inline coro <T> get () requires (!std::is_void_v <T>){
        co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));
        co_return std::move (data);
    }

    inline coro <void> wait () {
        co_await m_waiter.async_wait(as_tuple(boost::asio::use_awaitable));
    }

    awaitable_future (awaitable_future&&) noexcept = delete;
    awaitable_future (const awaitable_future&) = delete;
    awaitable_future& operator = (const awaitable_future&) = delete;
    awaitable_future& operator = (const awaitable_future&&) = delete;

    ~awaitable_future () {
        m_waiter.cancel();
    }
};

template <typename T>
class awaitable_promise {
    awaitable_future <T> m_future;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc): m_future (ioc) {}

    inline void set (T&& data) {
        m_future.data = std::move (data);
        m_future.m_waiter.cancel();
    }

    coro <T> get () {
        co_return m_future.get();
    }

    [[nodiscard]] inline const awaitable_future <T>& get_future () const {
        return m_future;
    }
};

template <>
class awaitable_promise <void> {
    awaitable_future <void> m_future;

public:
    explicit awaitable_promise(boost::asio::io_context& ioc): m_future (ioc) {}

    inline void set () {
        m_future.m_waiter.cancel();
    }

    coro <void> get () {
        co_await m_future.wait ();
    }

    [[nodiscard]] inline const awaitable_future <void>& get_future () const {
        return m_future;
    }
};

}

#endif //UH_CLUSTER_AWAITABLE_FUTURE_H
