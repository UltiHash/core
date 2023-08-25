#ifndef REST_NODE_SRC_SERVER
#define REST_NODE_SRC_SERVER

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
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
#include "logging/logging_boost.h"
#include "net/server.h"
#include "../cluster_config.h"

//------------------------------------------------------------------------------

namespace uh::rest
{

    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace http = beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;            // from <boost/asio.hpp>
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    using tcp_stream = typename beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

//------------------------------------------------------------------------------

    struct rest_server_config
    {
        constexpr static uint16_t DEFAULT_PORT = 8080;
        constexpr static std::size_t DEFAULT_THREADS = 5;

        std::size_t threads = DEFAULT_THREADS;
        boost::asio::ip::address address;
        uint16_t port = DEFAULT_PORT;
    };

//------------------------------------------------------------------------------

    class rest_server
    {
    private:
        uh::cluster::server_config m_config;
        net::io_context m_ioc;
        std::vector<std::thread> m_thread_container {};
        const boost::asio::ip::address m_server_address = boost::asio::ip::make_address("0.0.0.0");

    public:
        explicit rest_server(uh::cluster::server_config config);

        void run();

        // Handles an HTTP server connection
        net::awaitable<void> do_session(tcp_stream stream);

        // Accepts incoming connections and launches the sessions
        net::awaitable<void> do_listen(tcp::endpoint endpoint);
    };

//------------------------------------------------------------------------------

} // namespace uh::net

#endif // REST_NODE_SRC_SERVER
