#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/config.hpp>

#include <common/coroutines/coro_util.h>
#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/protocol_handler.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace uh::cluster {

struct server_config {
    uint16_t port;
    std::string bind_address;
};

class server {

public:
    server(server_config config, std::unique_ptr<protocol_handler> handler,
           boost::asio::io_context& ioc)
        : m_config(std::move(config)),
          m_ioc(ioc),
          m_handler(std::move(handler)),
          m_task{std::make_unique<coro_task>("do_accept", ioc)} {
        m_task->spawn(do_accept());
    }

    [[nodiscard]] const server_config& get_server_config() const {
        return m_config;
    }

    ~server() {
        LOG_INFO() << "stopping server...";
        m_task.reset();

        LOG_INFO() << "canceling sessions...";
        std::unique_lock<std::mutex> lock(m_sessions_mutex);
        for (auto& s : m_sessions) {
            s.cancel();
        }

        LOG_INFO() << "waiting sessions to be killed...";
        m_sessions_cv.wait(lock, [&] {
            LOG_DEBUG() << "session size: " << m_sessions.size();
            return m_sessions.empty();
        });
        LOG_INFO() << "server destroyed";
    }

private:
    class session {
    public:
        session(std::string name, boost::asio::io_context& ioc,
                boost::asio::ip::tcp::socket&& socket)
            : m_task{std::move(name), ioc},
              m_socket{std::move(socket)} {}

        auto& task() { return m_task; }
        auto& socket() { return m_socket; }
        void cancel() {
            m_task.cancel();
            m_socket.cancel();
        }

    private:
        coro_task m_task;
        boost::asio::ip::tcp::socket m_socket;
    };

    auto& emplace(std::string name, boost::asio::io_context& ioc,
                  boost::asio::ip::tcp::socket&& socket) {
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        return m_sessions.emplace_back(name, ioc, std::move(socket));
    }

    void erase(std::list<session>::const_iterator it) {
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        if (it != m_sessions.end())
            m_sessions.erase(it);
    }

    boost::asio::ip::tcp::acceptor
    do_listen(const boost::asio::ip::tcp::endpoint& endpoint) {
        auto acceptor =
            boost::asio::use_awaitable_t<boost::asio::any_io_executor>::
                as_default_on(boost::asio::ip::tcp::acceptor(m_ioc));

        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));

        acceptor.bind(endpoint);
        acceptor.listen(boost::asio::socket_base::max_listen_connections);
        return acceptor;
    }

    coro<void> do_accept() {

        LOG_INFO() << "server config: " << m_config.bind_address << ":"
                   << m_config.port;

        auto acceptor = do_listen(boost::asio::ip::tcp::endpoint{
            boost::asio::ip::make_address(m_config.bind_address),
            m_config.port});

        LOG_INFO() << "starting server, listening at " << m_config.bind_address
                   << ":" << m_config.port;

        auto state = co_await boost::asio::this_coro::cancellation_state;
        while (state.cancelled() == boost::asio::cancellation_type::none) {
            boost::asio::ip::tcp::socket s =
                co_await acceptor.async_accept(boost::asio::use_awaitable);

            std::string name = [&s]() {
                const auto conn_address =
                    s.remote_endpoint().address().to_string();
                const auto conn_port = s.remote_endpoint().port();

                return std::format("in session {}:{}", conn_address, conn_port);
            }();

            auto& session = emplace(name, m_ioc, std::move(s));
            auto& socket = session.socket();
            session.task().spawn(
                [this, &socket]() mutable -> coro<void> {
                    auto ep = socket.remote_endpoint();
                    LOG_INFO() << "connection from: " << ep;

                    counter_guard<active_connections> guard;

                    co_await m_handler->handle(socket);

                    LOG_INFO() << "session ended: " << ep;
                },

                [this, session_it =
                           std::prev(m_sessions.end())](std::exception_ptr e) {
                    LOG_DEBUG()
                        << "session finished: "
                        << std::distance(m_sessions.begin(), session_it);
                    erase(session_it);
                    m_sessions_cv.notify_all();
                });
        }
    }

    const server_config m_config;
    boost::asio::io_context& m_ioc;
    std::unique_ptr<protocol_handler> m_handler;

    std::mutex m_sessions_mutex;
    std::condition_variable m_sessions_cv;
    std::list<session> m_sessions;
    std::unique_ptr<coro_task> m_task;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster
