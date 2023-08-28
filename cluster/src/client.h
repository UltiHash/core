//
// Created by masi on 8/26/23.
//

#ifndef CORE_CLIENT_H
#define CORE_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <span>
#include <deque>
#include <memory>
#include <condition_variable>
#include "common.h"
#include "messenger.h"

namespace uh::cluster {

class client {
    std::deque <std::shared_ptr<messenger>> m_messengers;
    std::condition_variable m_cv;
    std::mutex m;

public:
    client (boost::asio::io_context &ioc, const std::string &ip, const int port, const int connections) {
        for (int i = 0; i < connections; ++i) {
            m_messengers.emplace_back(std::make_shared<messenger> (ioc, ip, port));
        }
    }

    client (client&& cl) noexcept: m_messengers (std::move (cl.m_messengers)) {

    }

    boost::asio::awaitable<std::shared_ptr <messenger>> send (const message_types type, std::span<const char> data) {
        std::unique_lock<std::mutex> lk(m);
        // The only case where a thread might wait. This wait never happen if we have higher number of connections than the threads
        m_cv.wait(lk, [this]() { return !m_messengers.empty(); });
        auto messenger = m_messengers.front();
        m_messengers.pop_front();
        co_await messenger->send(type, data);
        m_messengers.emplace_back (messenger);
        m_cv.notify_one();
        co_return messenger;
    }

};

} // end namespace uh::cluster

#endif //CORE_CLIENT_H
