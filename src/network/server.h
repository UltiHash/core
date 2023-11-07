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
#include "common/cluster_config.h"
#include <common/log.h>
#include "messenger.h"
#include "common/protocol_handler.h"

//------------------------------------------------------------------------------

namespace uh::cluster
{

//------------------------------------------------------------------------------

    class server {

    public:
        server(server_config config, std::unique_ptr <protocol_handler> handler) :
                m_config (config),
                m_ioc (std::make_shared <boost::asio::io_context> (m_config.threads)),
                m_handler (std::move (handler)) {
            m_is_running = true;
            boost::asio::co_spawn(*m_ioc,
                                  do_listen(boost::asio::ip::tcp::endpoint{m_server_address, m_config.port}),
                                  [](const std::exception_ptr &e) {
                                      if (e)
                                          try {
                                              std::rethrow_exception(e);
                                          }
                                          catch (boost::system::system_error &e) {
                                              LOG_INFO() << "Server stopped.";
                                          }
                                          catch (std::exception &e) {
                                              LOG_ERROR() << "accept: " << e.what();
                                          }
                                  });
        }



        void run() {
            LOG_INFO() << "starting server";

            for (auto i = 0; i < m_config.threads - 1; i++)
                m_thread_container.emplace_back(
                        [&] {
                            m_ioc->run();
                        });

            // the calling thread is also running the I/O service
            m_ioc->run();
        }

        void stop() {
            m_is_running = false;
            m_ioc->stop();
        }

        [[nodiscard]] std::shared_ptr <boost::asio::io_context> get_executor () const {
            return m_ioc;
        }

        prometheus::Family<prometheus::Counter>& add_counter_family(const std::string& name,
                                                                 const std::string& help)
        {
            auto builder = prometheus::BuildCounter().Name(name).Help(help);

            return builder.Register(*m_registry);
        }


        prometheus::Family<prometheus::Gauge>& add_gauge_family(const std::string& name,
                                                             const std::string& help)
        {
            auto builder = prometheus::BuildGauge().Name(name).Help(help);

            return builder.Register(*m_registry);
        }

        ~server() {
            for (auto& thread: m_thread_container) {
                thread.join();
            }
            stop ();
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

            while (m_is_running) {
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
                                    LOG_ERROR() << "in session: [" << conn_address << ":" << conn_port << "] "
                                         << e.what();

                                }

                            if(m_handler->stop_received()) {
                                m_is_running = false;
                                try {
                                    acceptor.close();
                                    while(acceptor.is_open()) {};
                                } catch (boost::system::system_error &e) {
                                    std::cout << "do we get here?" << std::endl;
                                }
                            }
                        });
            }
        }

        boost::asio::awaitable<void> do_session(boost::asio::ip::tcp::socket stream) {
            const auto life_time = m_ioc;
            LOG_INFO() << "connection from: " << stream.remote_endpoint();
            co_await m_handler->handle(messenger(std::move(stream)));

            co_return;
        }


        server_config m_config;
        std::shared_ptr <boost::asio::io_context> m_ioc;
        std::vector<std::thread> m_thread_container {};
        std::vector<boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::use_awaitable_t<boost::asio::any_io_executor>::executor_with_default<boost::asio::any_io_executor>>> m_acceptors;
        std::unique_ptr <protocol_handler> m_handler;
        const boost::asio::ip::address m_server_address = boost::asio::ip::make_address("0.0.0.0");
        std::atomic<bool> m_is_running;
        std::shared_ptr<prometheus::Registry> m_registry;
    };

//------------------------------------------------------------------------------

} // namespace uh::cluster


#endif //CORE_SERVER_H
