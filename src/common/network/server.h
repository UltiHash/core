#pragma once

#include "common/telemetry/log.h"
#include "common/telemetry/metrics.h"
#include "common/utils/protocol_handler.h"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
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
    uint16_t port;
    std::string bind_address;
};

class server {

public:
    server(server_config config, std::unique_ptr<protocol_handler> handler,
           boost::asio::io_context& ioc)
        : m_config(std::move(config)),
          m_ioc(ioc),
          m_handler(std::move(handler)) {

        LOG_INFO() << "server config: " << m_config.bind_address << ":"
                   << m_config.port;

        auto acceptor = do_listen(boost::asio::ip::tcp::endpoint{
            boost::asio::ip::make_address(m_config.bind_address),
            m_config.port});
        boost::asio::co_spawn(
            m_ioc, do_accept(std::move(acceptor)),
            [](const std::exception_ptr& e) {
                LOG_ERROR() << "acceptor error";
                if (e)
                    try {
                        std::rethrow_exception(e);
                    } catch (const boost::system::system_error& e) {
                        LOG_INFO() << "stopped server ";
                    } catch (const std::exception& e) {
                        LOG_ERROR() << "accept: " << e.what();
                    }
            });
        LOG_INFO() << "starting server, listening at " << m_config.bind_address
                   << ":" << m_config.port;
    }

    [[nodiscard]] const server_config& get_server_config() const {
        return m_config;
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

    coro<void> do_accept(auto acceptor) {
        while (true) {
            boost::asio::ip::tcp::socket stream =
                co_await acceptor.async_accept(boost::asio::use_awaitable);
            const auto conn_address =
                stream.remote_endpoint().address().to_string();
            const auto conn_port = stream.remote_endpoint().port();

            boost::asio::co_spawn(
                m_ioc, do_session(std::move(stream)),
                [conn_address, conn_port](const std::exception_ptr& e) {
                    if (e)
                        try {
                            std::rethrow_exception(e);
                        } catch (const std::exception& e) {
                            LOG_ERROR() << "in session: [" << conn_address
                                        << ":" << conn_port << "] " << e.what();
                        }
                });
        }
    }

    coro<void> do_session(boost::asio::ip::tcp::socket stream) {
        auto ep = stream.remote_endpoint();
        LOG_INFO() << "connection from: " << ep;
        counter_guard<active_connections> guard;
        co_await m_handler->handle(std::move(stream));
        LOG_INFO() << "session ended: " << ep;
        co_return;
    }

    const server_config m_config;
    boost::asio::io_context& m_ioc;

    std::vector<boost::asio::basic_socket_acceptor<
        boost::asio::ip::tcp,
        boost::asio::use_awaitable_t<>::executor_with_default<
            boost::asio::any_io_executor>>>
        m_acceptors;
    std::unique_ptr<protocol_handler> m_handler;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster
