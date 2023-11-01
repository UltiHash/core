//
// Created by masi on 10/31/23.
//

#ifndef CORE_AWAITABLE_CONDITION_VARIABLE_H
#define CORE_AWAITABLE_CONDITION_VARIABLE_H

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/lockfree/queue.hpp>
#include "common_types.h"
#include <queue>

namespace uh::cluster {

struct awaitable_condition_variable {
    std::shared_ptr <boost::asio::io_context> m_ioc;
    std::queue <std::shared_ptr <boost::asio::steady_timer>> m_waiters;
    std::shared_ptr <boost::asio::steady_timer> m_main_timer;
    std::atomic_flag m_flag;
    std::atomic_flag m_running;

    explicit awaitable_condition_variable (std::shared_ptr <boost::asio::io_context> ioc):
        m_ioc (std::move (ioc)),
        m_running (false) {
        m_flag.clear();
    }

    awaitable_condition_variable (awaitable_condition_variable&& cv) noexcept:
        m_ioc (std::move (cv.m_ioc)),
        m_waiters (std::move (cv.m_waiters)),
        m_flag (cv.m_flag.test()) {}

    boost::asio::awaitable <void> wait () {

        auto waiter = std::make_shared <boost::asio::steady_timer> (*m_ioc, boost::asio::steady_timer::clock_type::duration::max());
        while (!m_flag.test_and_set());
        if (!m_running.test() and m_waiters.empty()) {
            m_running.test_and_set();
            m_flag.clear();
            co_return;
        }
        m_waiters.emplace(waiter);
        m_flag.clear();
        co_await waiter->async_wait(as_tuple(boost::asio::use_awaitable));
        int i= 0;
    }

    void notify () {
        m_running.clear();

        if (!m_waiters.empty()) {
            m_waiters.front()->cancel();
            m_waiters.pop();
        }

    }

};
} // end namespace uh::cluster

#endif //CORE_AWAITABLE_CONDITION_VARIABLE_H
