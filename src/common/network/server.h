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
#include <unordered_set>
#include <utility>
#include <vector>

namespace uh::cluster {

struct server_config {
    uint16_t port;
    std::string bind_address;
};

class server {

public:
    server(server_config config,
           std::unique_ptr<protocol_handler_factory> handler_factory,
           boost::asio::io_context& ioc)
        : m_config(std::move(config)),
          m_ioc(ioc),
          m_handler_factory(std::move(handler_factory)),
          m_task{std::make_unique<coro_task>("do_accept", ioc)} {
        m_task->spawn(do_accept());
    }

    [[nodiscard]] const server_config& get_server_config() const {
        return m_config;
    }

    ~server() {
        LOG_INFO() << "stopping server...";
        m_task.reset();

        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOG_INFO() << "canceling sessions...";
        {
            std::unique_lock<std::mutex> lock(m_sessions_mutex);
            for (auto& session : m_sessions) {
                session->cancel();
            }
        }

        LOG_INFO() << "waiting sessions to be killed...";
        std::unique_lock<std::mutex> lock(m_sessions_mutex);
        m_sessions_cv.wait(lock, [&] {
            LOG_DEBUG() << "session size: " << m_sessions.size();
            return m_sessions.empty();
        });
        LOG_INFO() << "clear sessions";
        m_sessions.clear();
        LOG_INFO() << "clear handler factory";
        m_handler_factory.reset();
        LOG_INFO() << "server destroyed";
    }

private:
    void create_session(std::string name, boost::asio::io_context& ioc,
                        std::unique_ptr<protocol_handler> handler) {
        std::lock_guard<std::mutex> lock(m_sessions_mutex);

        auto [it, inserted] =
            m_sessions.emplace(std::make_shared<coro_task>(name, ioc));
        if (inserted == false) {
            LOG_ERROR() << "session with name '" << name
                        << "' already exists, cannot create a new one";
            return;
        }
        (*it)->spawn(
            [handler = std::move(handler)]() mutable -> coro<void> {
                counter_guard<active_connections> guard;
                co_await handler->run();
            },
            // session should alive until the completion handler removes it
            [this, self = *it](std::exception_ptr _) { remove_session(self); });
    }

    void remove_session(std::shared_ptr<coro_task> session) {
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        try {
            LOG_DEBUG() << "removing session";
            auto it = m_sessions.find(session);
            if (it != m_sessions.end()) {
                m_sessions.erase(it);
                LOG_DEBUG()
                    << "session removed, remaining: " << m_sessions.size();
                m_sessions_cv.notify_all();
            }
        } catch (const std::exception& e) {
            LOG_ERROR() << "failed to remove session: " << e.what();
        }
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

                return std::format("session {}:{}", conn_address, conn_port);
            }();

            create_session(name, m_ioc,
                           m_handler_factory->create_handler(std::move(s)));
        }
    }

    const server_config m_config;
    boost::asio::io_context& m_ioc;
    std::unique_ptr<protocol_handler_factory> m_handler_factory;

    std::mutex m_sessions_mutex;
    std::condition_variable m_sessions_cv;
    std::unordered_set<std::shared_ptr<coro_task>> m_sessions;
    std::unique_ptr<coro_task> m_task;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster
