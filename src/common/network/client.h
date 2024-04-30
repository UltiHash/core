#ifndef CORE_CLIENT_H
#define CORE_CLIENT_H

#include "common/coroutines/awaitable_promise.h"
#include "common/utils/common.h"
#include "messenger.h"
#include "tools.h"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <condition_variable>
#include <deque>
#include <memory>
#include <span>

namespace uh::cluster {

class client;

class acquired_messenger {
public:
    acquired_messenger(std::unique_ptr<messenger> m, client& cl)
        : m_messenger(std::move(m)),
          m_client(cl) {}

    acquired_messenger(acquired_messenger&& m) noexcept
        : m_messenger(std::move(m.m_messenger)),
          m_client(m.m_client) {}

    [[nodiscard]] messenger& get() const { return *m_messenger; }

    messenger* operator->() { return m_messenger.get(); }
    messenger& operator*() { return *m_messenger; }

    ~acquired_messenger() { release(); }

private:
    void inline release();

    std::unique_ptr<messenger> m_messenger;
    client& m_client;
};

class client {
public:
    client(boost::asio::io_context& ioc, const std::string& address,
           const std::uint16_t port, const int connections)
        : m_ioc(ioc) {

        auto endpoint = resolve(address, port).back();

        for (int i = 0; i < connections; ++i) {
            m_messengers.emplace_back(std::make_unique<messenger>(
                ioc, endpoint.address().to_string(), port));
        }
    }

    client(client&& cl) noexcept
        : m_ioc(cl.m_ioc),
          m_messengers(std::move(cl.m_messengers)) {}

    coro<acquired_messenger> acquire_messenger() {
        std::shared_ptr<awaitable_promise<acquired_messenger>> promise;
        {
            std::unique_lock<std::mutex> lk(m);

            if (!m_messengers.empty()) {

                auto messenger = std::move(m_messengers.front());
                m_messengers.pop_front();
                lk.unlock();

                co_return acquired_messenger(std::move(messenger),
                                             std::ref(*this));
            }

            promise =
                std::make_shared<awaitable_promise<acquired_messenger>>(m_ioc);
            m_promises.push_back(promise);
        }

        co_return co_await promise->get();
    }

private:
    friend class acquired_messenger;

    boost::asio::io_context& m_ioc;
    std::mutex m;
    std::deque<std::unique_ptr<messenger>> m_messengers;
    std::list<std::shared_ptr<awaitable_promise<acquired_messenger>>>
        m_promises;

    void push_messenger(std::unique_ptr<messenger> msg) {
        std::unique_lock<std::mutex> lk(m);

        if (!m_promises.empty()) {
            auto promise = m_promises.front();
            m_promises.pop_front();

            promise->set(acquired_messenger(std::move(msg), *this));
            return;
        }

        m_messengers.emplace_back(std::move(msg));
    }
};

void inline acquired_messenger::release() {
    if (m_messenger) {
        m_messenger->clear_buffers();
        m_client.push_messenger(std::move(m_messenger));
    }
}

} // end namespace uh::cluster

#endif // CORE_CLIENT_H
