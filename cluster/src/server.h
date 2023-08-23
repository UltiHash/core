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
#include <boost/beast/http/message_generator.hpp>
#include <logging/logging_boost.h>
#include "cluster_config.h"

//------------------------------------------------------------------------------

namespace uh::cluster
{

    namespace net = boost::asio;            // from <boost/asio.hpp>
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------



//------------------------------------------------------------------------------

    class server {

    public:
        explicit server(server_config config) :
                m_config(config), m_ioc(static_cast<int>(m_config.threads)),
                m_thread_container(m_config.threads - 1) {
            boost::asio::co_spawn(m_ioc,
                                  do_listen(tcp::endpoint{m_server_address, m_config.port}),
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
        net::awaitable<void> do_listen(tcp::endpoint endpoint) const {
            auto acceptor = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
                    tcp::acceptor(co_await net::this_coro::executor));

            acceptor.open(endpoint.protocol());
            acceptor.set_option(net::socket_base::reuse_address(true));

            acceptor.bind(endpoint);
            acceptor.listen(net::socket_base::max_listen_connections);

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

        net::awaitable<void> do_session(tcp::socket stream) const {
            INFO << "connection from: " << stream.remote_endpoint();

            auto buffer = std::make_unique_for_overwrite<char[]> (m_config.buffer_size);
            boost::system::error_code ec;
            constexpr auto nothrow_await = boost::asio::as_tuple(boost::asio::use_awaitable);
            try {
                for (;;) {
                    co_await boost::asio::async_read(stream, boost::asio::buffer(buffer.get(), m_config.buffer_size), nothrow_await);
                   // co_await boost::asio::this_coro::async_read_some (stream, buffer, net::use_awaitable);
                    std::cout << "message " << std::string_view (buffer.get(), 10) << std::endl;
                }
            }
            catch (boost::system::system_error &se) {

            }

            stream.shutdown(tcp::socket::shutdown_send, ec);
        }

        server_config m_config;
        net::io_context m_ioc;
        std::vector<std::thread> m_thread_container {};
        const boost::asio::ip::address m_server_address = boost::asio::ip::make_address("0.0.0.0");

    };

//------------------------------------------------------------------------------

} // namespace uh::cluster


#endif //CORE_SERVER_H
