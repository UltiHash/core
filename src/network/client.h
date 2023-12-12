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
#include "common/common.h"
#include "messenger.h"

namespace uh::cluster {



class client {

public:

    class acquired_messenger {
    public:
        acquired_messenger (std::unique_ptr <messenger> m,
                            std::reference_wrapper <client> cl):
                m_messenger(std::move (m)),
                m_client(cl),
                m_owning (true) {
        }

        acquired_messenger (acquired_messenger&& m) noexcept:
                m_messenger (std::move (m.m_messenger)), m_client (m.m_client), m_owning (true) {
            m.m_owning = false;
        }

        [[nodiscard]] messenger& get () const {
            return *m_messenger;
        }

        void release () {
            m_messenger->clear_buffers();
            m_client.get().push_messenger(std::move(m_messenger));
            m_owning = false;
        }

        ~acquired_messenger() {
            if(m_owning) {
                release();
            }
        }

        std::unique_ptr <messenger> m_messenger;
        const std::reference_wrapper <client> m_client;
        bool m_owning;
    };

    std::deque <std::unique_ptr<messenger>> m_messengers;
    std::condition_variable m_cv;
    std::mutex m;

    client (const std::shared_ptr <boost::asio::io_context>& ioc, const std::string &address, const int port, const int connections) {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(address, std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
        boost::asio::ip::tcp::resolver::iterator end; // End marker.
        boost::asio::ip::tcp::endpoint endpoint;
        while (iter != end)
        {
            endpoint = iter->endpoint();
            iter++;
        }
        for (int i = 0; i < connections; ++i) {
            m_messengers.emplace_back(std::make_unique<messenger> (ioc, endpoint));
        }
    }

    client (client&& cl) noexcept: m_messengers (std::move (cl.m_messengers)) {

    }

    acquired_messenger acquire_messenger () {
        std::unique_lock<std::mutex> lk(m);
        m_cv.wait(lk, [this]() { return !m_messengers.empty(); });
        auto messenger = std::move (m_messengers.front());
        m_messengers.pop_front();
        return acquired_messenger {std::move (messenger), std::ref (*this)};
    }

private:
    void push_messenger (std::unique_ptr <messenger> msg) {
        std::unique_lock <std::mutex> lk (m);
        m_messengers.emplace_back(std::move (msg));
        lk.unlock();
        m_cv.notify_one();
    }

};

} // end namespace uh::cluster

#endif //CORE_CLIENT_H
