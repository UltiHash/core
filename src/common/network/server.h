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

class session_runner : public std::enable_shared_from_this<session_runner> {
public:
    session_runner(
        std::function<coro<void>(boost::asio::ip::tcp::socket m)> handler)
        : m_session_handler{handler} {}

    void run(std::string name, boost::asio::io_context& ioc,
             boost::asio::ip::tcp::socket stream,
             std::function<void(std::shared_ptr<session_runner>)> on_finish) {
        auto self = shared_from_this();
        m_task = std::make_unique<coro_task>(
            std::move(name), ioc, do_session(std::move(stream)),
            [self, on_finish = std::move(on_finish)](std::exception_ptr _) {
                on_finish(self);
            });
    }

    void cancel() {
        if (m_task) {
            m_task->cancel();
        }
    }

    void wait() {
        if (m_task) {
            m_task->wait();
        }
    }

    coro<void> do_session(boost::asio::ip::tcp::socket stream) {
        auto ep = stream.remote_endpoint();
        LOG_INFO() << "connection from: " << ep;
        counter_guard<active_connections> guard;
        co_await m_session_handler(std::move(stream));
        LOG_INFO() << "session ended: " << ep;
        co_return;
    }

private:
    std::function<coro<void>(boost::asio::ip::tcp::socket m)> m_session_handler;
    std::unique_ptr<coro_task> m_task;
};

class server {

public:
    server(server_config config, std::unique_ptr<protocol_handler> handler,
           boost::asio::io_context& ioc)
        : m_config(std::move(config)),
          m_ioc(ioc),
          m_handler(std::move(handler)),
          m_task{std::make_unique<coro_task>("do_accept", ioc, do_accept())} {}

    [[nodiscard]] const server_config& get_server_config() const {
        return m_config;
    }

    ~server() {
        m_task.reset();

        std::unique_lock<std::mutex> lock(m_sessions_mutex);
        for (const auto& session : m_sessions) {
            session->cancel();
        }
        m_sessions_cv.wait(lock, [&] { return m_sessions.empty(); });
    }

private:
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
            boost::asio::ip::tcp::socket stream =
                co_await acceptor.async_accept(boost::asio::use_awaitable);
            const auto conn_address =
                stream.remote_endpoint().address().to_string();
            const auto conn_port = stream.remote_endpoint().port();
            std::string name =
                std::format("in session {}:{}", conn_address, conn_port);
            auto session = std::make_shared<session_runner>(
                [&](auto stream) -> coro<void> {
                    co_await m_handler->handle(std::move(stream));
                });

            session->run(name, m_ioc, std::move(stream),
                         [this](std::shared_ptr<session_runner> session) {
                             remove_session(session);
                             m_sessions_cv.notify_all();
                         });
            add_session(session);
        }
    }

    void add_session(const std::shared_ptr<session_runner>& session) {
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        m_sessions.push_back(session);
    }

    void remove_session(const std::shared_ptr<session_runner>& session) {
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        auto it = std::find(m_sessions.begin(), m_sessions.end(), session);
        if (it != m_sessions.end())
            m_sessions.erase(it);
    }

    const server_config m_config;
    boost::asio::io_context& m_ioc;
    std::unique_ptr<protocol_handler> m_handler;

    std::mutex m_sessions_mutex;
    std::condition_variable m_sessions_cv;
    std::list<std::shared_ptr<session_runner>> m_sessions;
    std::unique_ptr<coro_task> m_task;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster
