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
#include <utility>
#include <vector>
#include "common/utils/cluster_config.h"
#include "common/utils/log.h"
#include "messenger.h"
#include "common/utils/protocol_handler.h"

//------------------------------------------------------------------------------

namespace uh::cluster
{

//------------------------------------------------------------------------------

    class server {

    public:
        server(server_config config, std::string server_name, std::unique_ptr <protocol_handler> handler) :
                m_config (std::move(config)),
                m_ioc (std::make_shared <boost::asio::io_context> (m_config.threads)),
                m_handler (std::move (handler)),
                m_server_name (std::move(server_name)) {
            m_is_running = true;

            auto acceptor = do_listen(boost::asio::ip::tcp::endpoint{boost::asio::ip::make_address(m_config.bind_address), m_config.port});
            boost::asio::co_spawn(*m_ioc,

                                  do_accept (std::move (acceptor)),
                                  [&](const std::exception_ptr &e) {
                                      if (e)
                                          try {
                                              std::rethrow_exception(e);
                                          }
                                          catch (boost::system::system_error &e) {
                                              LOG_INFO() << "stopped server " << m_server_name << std::endl;
                                          }
                                          catch (std::exception &e) {
                                              LOG_ERROR() << "accept: " << e.what();
                                          }
                                  });
        }



        void run() {
            m_handler->init();

            LOG_INFO() << "starting server " << m_server_name << ", listening at " << m_config.bind_address << ":" << m_config.port;
            std::exception_ptr excp_ptr;

            for (size_t i = 0; i < m_config.threads - 1; i++)
                m_thread_container.emplace_back(
                        [&] {
                            try {
                                m_ioc->run();
                            } catch
                            (std::exception& e) {
                                excp_ptr = std::current_exception();
                            }
                        });

            // the calling thread is also running the I/O service
            try {
                m_ioc->run();
            } catch
                    (std::exception& e) {
                excp_ptr = std::current_exception();
            }

            if (excp_ptr) {
                try {
                    std::rethrow_exception(excp_ptr);
                }
                catch (std::exception& e) {
                    throw e;
                }
            }
        }

        void stop() {
            m_is_running = false;
            m_ioc->stop();
        }

        [[nodiscard]] std::shared_ptr <boost::asio::io_context> get_executor () const {
            return m_ioc;
        }

        [[nodiscard]] const server_config& get_server_config() const {
            return m_config;
        }

        ~server() {
            for (auto& thread: m_thread_container) {
                thread.join();
            }
            stop ();
        }

    private:

        boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::use_awaitable_t<boost::asio::any_io_executor>::executor_with_default<boost::asio::any_io_executor>>
        do_listen (const boost::asio::ip::tcp::endpoint& endpoint) {
            auto acceptor = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
                    boost::asio::ip::tcp::acceptor(*m_ioc));

            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::socket_base::reuse_address(true));

            acceptor.bind(endpoint);
            acceptor.listen(boost::asio::socket_base::max_listen_connections);
            return acceptor;
        }
        
        // Accepts incoming connections and launches the sessions
        boost::asio::awaitable<void> do_accept (auto acceptor) {


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
                                    LOG_ERROR() << "Error in closing the server acceptor in " << m_server_name;
                                }
                            }
                        });
            }
        }

        boost::asio::awaitable<void> do_session(boost::asio::ip::tcp::socket stream) {
            LOG_INFO() << m_server_name << " connection from: " << stream.remote_endpoint();
            co_await m_handler->handle (messenger(std::move(stream)));
            co_return;
        }


        const server_config m_config;
        std::shared_ptr <boost::asio::io_context> m_ioc;
        std::vector<std::thread> m_thread_container {};
        std::vector<boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::use_awaitable_t<boost::asio::any_io_executor>::executor_with_default<boost::asio::any_io_executor>>> m_acceptors;
        std::unique_ptr <protocol_handler> m_handler;
        std::atomic<bool> m_is_running;
        const std::string m_server_name;
    };

//------------------------------------------------------------------------------

} // namespace uh::cluster


#endif //CORE_SERVER_H
