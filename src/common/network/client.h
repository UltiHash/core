#ifndef CORE_CLIENT_H
#define CORE_CLIENT_H

#include "common/utils/awaitable_promise.h"
#include "common/utils/common.h"
#include "messenger.h"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <condition_variable>
#include <deque>
#include <memory>
#include <span>

namespace uh::cluster {

class client {

public:
    class acquired_messenger {
    public:
        acquired_messenger(std::unique_ptr<messenger> m,
                           const std::reference_wrapper<client> cl)
            : m_messenger(std::move(m)),
              m_client(cl) {}

        acquired_messenger(acquired_messenger&& m) = default;

        [[nodiscard]] messenger& get() const { return *m_messenger; }

        void release() {
            if (m_messenger) {
                m_messenger->clear_buffers();
                m_client.get().push_messenger(std::move(m_messenger));
            }
        }

        ~acquired_messenger() { release(); }

        std::unique_ptr<messenger> m_messenger;
        const std::reference_wrapper<client> m_client;
    };

    std::deque<std::unique_ptr<messenger>> m_messengers;
    std::condition_variable m_cv;
    std::mutex m;

    client(boost::asio::io_context& ioc, const std::string& address,
           const std::uint16_t port, const int connections)
        : m_coro_acqu([this]() { coro_acqu_runner(); }) {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(address,
                                                    std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
        boost::asio::ip::tcp::resolver::iterator end; // End marker.
        boost::asio::ip::tcp::endpoint endpoint;
        while (iter != end) {
            endpoint = iter->endpoint();
            iter++;
        }
        for (int i = 0; i < connections; ++i) {
            m_messengers.emplace_back(std::make_unique<messenger>(
                ioc, endpoint.address().to_string(), port));
        }
    }

    client(client&& cl) noexcept
        : m_messengers(std::move(cl.m_messengers)) {}

    ~client() noexcept {
        m_running = false;
        std::unique_lock<std::mutex> lk(m_coro_mutex);
        m_coro_cv.notify_all();
        m_coro_acqu.join();
    }

    acquired_messenger acquire_messenger() {
        std::unique_lock<std::mutex> lk(m);
        m_cv.wait(lk, [this]() { return !m_messengers.empty(); });
        auto messenger = std::move(m_messengers.front());
        m_messengers.pop_front();
        return acquired_messenger{std::move(messenger), std::ref(*this)};
    }

    coro<std::shared_ptr<acquired_messenger>>
    coro_acquire(boost::asio::io_context& ioc) {

        std::shared_ptr<awaitable_promise<std::shared_ptr<acquired_messenger>>>
            promise = std::make_shared<
                awaitable_promise<std::shared_ptr<acquired_messenger>>>(ioc);

        {
            std::unique_lock<std::mutex> lk(m_coro_mutex);
            m_coro_list.push_back(promise);
        }

        m_coro_cv.notify_one();
        return promise->get();
    }

private:
    void coro_acqu_runner() {
        m_running = true;
        while (m_running) {
            std::shared_ptr<
                awaitable_promise<std::shared_ptr<acquired_messenger>>>
                promise;
            {
                std::unique_lock<std::mutex> lk(m_coro_mutex);
                m_coro_cv.wait(lk, [this]() { return !m_coro_list.empty(); });
                promise = m_coro_list.front();
                m_coro_list.pop_front();
            }

            try {
                auto m = acquire_messenger();
                auto sp = std::make_shared<acquired_messenger>(std::move(m));
                promise->set(std::move(sp));
            } catch (const std::exception&) {
                promise->set_exception(std::current_exception());
            }
        }
    }

    std::thread m_coro_acqu;
    std::list<
        std::shared_ptr<awaitable_promise<std::shared_ptr<acquired_messenger>>>>
        m_coro_list;
    std::mutex m_coro_mutex;
    std::condition_variable m_coro_cv;
    std::atomic<bool> m_running;

    void push_messenger(std::unique_ptr<messenger> msg) {
        std::unique_lock<std::mutex> lk(m);
        if (!msg) {
            LOG_WARN() << "*** messenger is nullptr!";
            msg->reset_read_buffers();
        }
        m_messengers.emplace_back(std::move(msg));
        lk.unlock();
        m_cv.notify_one();
    }
};

} // end namespace uh::cluster

#endif // CORE_CLIENT_H
