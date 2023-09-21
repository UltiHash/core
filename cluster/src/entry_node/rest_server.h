#ifndef REST_NODE_SRC_SERVER
#define REST_NODE_SRC_SERVER

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
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
#include "common/cluster_config.h"
#include "common/protocol_handler.h"
#include "entry_node_rest_handler.h"
#include "network/client.h"
#include <map>

//------------------------------------------------------------------------------

namespace uh::cluster
{

    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace http = beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;            // from <boost/asio.hpp>
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    using tcp_stream = typename beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

//------------------------------------------------------------------------------

    class rest_server
    {
    private:
        server_config m_config;
        std::shared_ptr <net::io_context> m_ioc;
        std::vector<std::thread> m_thread_container {};
        boost::asio::ssl::context m_ssl;
        entry_node_rest_handler m_handler;
        bool m_server_busy = false;
        std::atomic<int> some_number = 0;
        //TODO: remove the m_is_close for when multiple same requests arrive
        // Handle multiple same requests arriving
        std::atomic<bool> m_is_close = false;

        // TODO: WHY does it WORK without mutex protection????
        std::map<uint16_t, std::string> m_multi_part_container;
        std::mutex m_mutex;

        const boost::asio::ip::address m_server_address = boost::asio::ip::make_address("0.0.0.0");

    public:
        rest_server(server_config config, std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes);

        void run();

        [[nodiscard]] std::shared_ptr <boost::asio::io_context> get_executor () const;

        ~rest_server();

        // Handles an HTTP server connection
        net::awaitable<void> do_session(tcp_stream stream);

        // Accepts incoming connections and launches the sessions
        net::awaitable<void> do_listen(tcp::endpoint endpoint);
    };

//------------------------------------------------------------------------------

} // namespace uh::cluster

#endif // REST_NODE_SRC_SERVER
