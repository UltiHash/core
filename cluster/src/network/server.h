//
// Created by masi on 8/16/23.
//

#ifndef CORE_SERVER_H
#define CORE_SERVER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "logging/logging_boost.h"
#include "common/cluster_config.h"
#include "messenger.h"
#include "common/protocol_handler.h"

//------------------------------------------------------------------------------

namespace uh::cluster
{

//------------------------------------------------------------------------------

    class server {

    public:
        server(server_config config, std::unique_ptr <protocol_handler> handler) :
                m_config (config), m_ioc (m_config.threads),
                m_thread_container (m_config.threads - 1), m_handler (std::move (handler)) {

            boost::asio::co_spawn(m_ioc,
                                  do_listen(boost::asio::ip::tcp::endpoint{m_server_address, m_config.port}),
                                  [](const std::exception_ptr &e) {
                                      if (e)
                                          try {
                                              std::rethrow_exception(e);
                                          }
                                          catch (std::exception &e) {
                                              std::cerr << "Error in acceptor: " << e.what() << "\n";
                                          }
                                  });
        }

        void run() {
            INFO << "starting server";

            for (auto i = 0; i < m_config.threads - 1; i++)
                m_thread_container.emplace_back(
                        [&] {
                            m_ioc.run();
                        });

            // the calling thread is also running the I/O service
            m_ioc.run();
        }

    private:

        // Accepts incoming connections and launches the sessions
        boost::asio::awaitable<void> do_listen(boost::asio::ip::tcp::endpoint endpoint) {
            auto acceptor = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
                    boost::asio::ip::tcp::acceptor(co_await boost::asio::this_coro::executor));

            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::socket_base::reuse_address(true));

            acceptor.bind(endpoint);
            acceptor.listen(boost::asio::socket_base::max_listen_connections);

            for (;;) {
                boost::asio::ip::tcp::socket stream = co_await acceptor.async_accept();
                auto conn_address = stream.remote_endpoint().address().to_string();
                auto conn_port = stream.remote_endpoint().port();

                boost::asio::co_spawn(
                        acceptor.get_executor(),
                        do_session(std::move(stream)),
                        [&](const std::exception_ptr &e) {
                            if (e)
                                try {
                                    std::rethrow_exception(e);
                                }
                                catch (const std::exception &e) {
                                    INFO << "Error in session: [" << conn_address << ":" << conn_port << "] "
                                         << e.what();
                                }
                        });
            }

        }

        boost::asio::awaitable<void> do_session(boost::asio::ip::tcp::socket stream) {
            INFO << "connection from: " << stream.remote_endpoint();
            m_handler->handle(messenger (m_ioc, std::move (stream)));
            co_return;
        }

        server_config m_config;
        boost::asio::io_context m_ioc;
        std::vector<std::thread> m_thread_container {};
        std::unique_ptr <protocol_handler> m_handler;
        const boost::asio::ip::address m_server_address = boost::asio::ip::make_address("0.0.0.0");

    };

//------------------------------------------------------------------------------

} // namespace uh::cluster


#endif //CORE_SERVER_H
