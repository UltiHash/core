#ifndef CORE_SERVER_H
#define CORE_SERVER_H

#include "common/telemetry/log.h"
#include "common/utils/protocol_handler.h"
#include <boost/asio.hpp>
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
    server(server_config config, std::unique_ptr<protocol_handler> handler,
           boost::asio::io_context& ioc)
        : m_config(std::move(config)),
          m_ioc(ioc),
          m_handler(std::move(handler)) {
        m_is_running = true;

        auto acceptor = do_listen(boost::asio::ip::tcp::endpoint{
            boost::asio::ip::make_address(m_config.bind_address),
            m_config.port});
        boost::asio::co_spawn(
            m_ioc, do_accept(std::move(acceptor)),
            [](const std::exception_ptr& e) {
                if (e)
                    try {
                        std::rethrow_exception(e);
                    } catch (const boost::system::system_error& e) {
                        LOG_INFO() << "stopped server ";
                    } catch (const std::exception& e) {
                        LOG_ERROR() << "accept: " << e.what();
                    }
            });
    }

    void run() {
        LOG_INFO() << "starting server, listening at " << m_config.bind_address
                   << ":" << m_config.port;
        std::exception_ptr excp_ptr;

        for (size_t i = 0; i < m_config.threads - 1; i++)
            m_thread_container.emplace_back([&] {
                try {
                    m_ioc.run();
                } catch (const std::exception&) {
                    excp_ptr = std::current_exception();
                }
            });

        // the calling thread is also running the I/O service
        try {
            m_ioc.run();
        } catch (std::exception& e) {
            excp_ptr = std::current_exception();
        }

        if (excp_ptr) {
            try {
                std::rethrow_exception(excp_ptr);
            } catch (std::exception& e) {
                throw e;
            }
        }
    }

    void stop() {
        LOG_INFO() << "stopping server ";
        m_is_running = false;
        m_ioc.stop();
    }

    [[nodiscard]] const server_config& get_server_config() const {
        return m_config;
    }

    ~server() {
        for (auto& thread : m_thread_container) {
            thread.join();
        }
    }

private:
    boost::asio::ip::tcp::acceptor
    do_listen(const boost::asio::ip::tcp::endpoint& endpoint);

    coro<void> do_accept(auto acceptor);

    coro<void> do_session(boost::asio::ip::tcp::socket stream);

    const server_config m_config;
    boost::asio::io_context& m_ioc;

    std::vector<std::thread> m_thread_container{};
    std::vector<boost::asio::basic_socket_acceptor<
        boost::asio::ip::tcp,
        boost::asio::use_awaitable_t<>::executor_with_default<
            boost::asio::any_io_executor>>>
        m_acceptors;
    std::unique_ptr<protocol_handler> m_handler;
    std::atomic<bool> m_is_running;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster

#endif // CORE_SERVER_H
