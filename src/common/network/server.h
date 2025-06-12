#pragma once

#include <common/execution/executor.h>
#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/protocol_handler.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace uh::cluster {

struct server_config {
    std::size_t threads;
    uint16_t port;
    std::string bind_address;
};

class server {

public:
    server(server_config config,
           std::unique_ptr<protocol_handler> handler,
           executor& exec)
        : m_config(std::move(config)),
          m_acceptor(do_listen({boost::asio::ip::make_address(m_config.bind_address),
            m_config.port}, exec.get_executor())),
          m_handler(std::move(handler)) {

        LOG_INFO() << "server config: " << m_config.bind_address << ":"
                   << m_config.port;

        exec.repeated(
            [this]() { m_acceptor.cancel(); },
            [&exec, this]() -> coro<void> {
                exec.spawn(&server::do_session, this, co_await m_acceptor.async_accept());
            });
    }

    [[nodiscard]] const server_config& get_server_config() const {
        return m_config;
    }

private:
    static boost::asio::ip::tcp::acceptor
    do_listen(const boost::asio::ip::tcp::endpoint& endpoint,
              boost::asio::io_context& ioc) {
        auto acceptor =
            boost::asio::use_awaitable_t<boost::asio::any_io_executor>::
                as_default_on(boost::asio::ip::tcp::acceptor(ioc));

        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));

        acceptor.bind(endpoint);
        acceptor.listen(boost::asio::socket_base::max_listen_connections);
        return acceptor;
    }

    coro<void> do_session(boost::asio::ip::tcp::socket stream) {
        auto remote = stream.remote_endpoint();
        LOG_INFO() << "connection from: " << remote;

        counter_guard<active_connections> guard;

        try {
            co_await m_handler->handle(std::move(stream));
        } catch (const std::exception& e) {
            LOG_ERROR() << "in session: [" << remote << "] " << e.what();
        }

        LOG_INFO() << "session ended: " << remote;
    }

    server_config m_config;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::unique_ptr<protocol_handler> m_handler;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster
