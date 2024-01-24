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

#include "common/utils/cluster_config.h"
#include "common/utils/protocol_handler.h"
#include "common/utils/services.h"
#include "common/network/client.h"

#include "entrypoint_rest_handler.h"
#include "rest/http/http_request.h"
#include "rest/http/http_response.h"
#include "rest/utils/state/server_state.h"
#include "common/registry/service_registry.h"

//------------------------------------------------------------------------------

namespace uh::cluster::rest
{

    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace b_http = beast::http;           // from <boost/beast/http.hpp>

    namespace net = boost::asio;            // from <boost/asio.hpp>
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    using tcp_stream = typename beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

//------------------------------------------------------------------------------


    class rest_server
    {
    private:
        boost::asio::io_context& m_ioc;

        const server_config m_config;

        std::vector<std::thread> m_thread_container {};
        entrypoint_rest_handler m_handler;

        utils::server_state m_server_state;

        boost::asio::ip::address m_server_address;

    public:
        rest_server(server_config config,
                    const services<DEDUPLICATOR_SERVICE>& dedupe_nodes,
                    const services<DIRECTORY_SERVICE>& directory_nodes,
                    std::shared_ptr <boost::asio::thread_pool> workers,
                    boost::asio::io_context& ioc);

        void run();

        [[nodiscard]] boost::asio::io_context&
        get_executor () const;

        [[nodiscard]] const server_config& get_server_config();

        ~rest_server();

        coro <void>
        recover_failed_nodes ();

        // Handles an HTTP server connection
        coro <void>
        do_session(tcp_stream stream);

        // Accepts incoming connections and launches the sessions
        coro <void>
        do_listen(tcp::endpoint endpoint);


    };

//------------------------------------------------------------------------------

} // namespace uh::cluster::rest


#endif // REST_NODE_SRC_SERVER
