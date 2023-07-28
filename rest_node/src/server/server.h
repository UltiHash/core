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
#include <logging/logging_boost.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using tcp_stream = typename beast::tcp_stream::rebind_executor<
        net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

//------------------------------------------------------------------------------

// Handles the request received
template <class Body, class Allocator>
http::response<http::string_body>
handle_request(http::request<Body, http::basic_fields<Allocator>>&& req)
{

    // Returns a bad request response
    auto const bad_request =
            [&req](beast::string_view why)
            {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };

    // Respond to HEAD request
    if(req.method() == http::verb::get)
    {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string("This is the response for the GET request.");
        res.prepare_payload();
        return res;
    }

    return bad_request("Unknown HTTP-method");
}

//------------------------------------------------------------------------------

// Handles an HTTP server connection
net::awaitable<void>
do_session(tcp_stream stream)
{
    INFO << "connection from: " << stream.socket().remote_endpoint();

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    // This lambda is used to send messages
    try
    {
        for(;;)
        {
            // Set the timeout.
            stream.expires_after(std::chrono::seconds(10));

            // Read a request
            http::request<http::string_body> req;
            co_await http::async_read(stream, buffer, req);

            // Handle the request
            http::response<http::string_body> msg =
                    handle_request(std::move(req));

            // Determine if we should close the connection
            bool keep_alive = msg.keep_alive();

            // Send the response
            co_await http::async_write(stream, std::move(msg), net::use_awaitable);

            if(! keep_alive)
            {
                // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                break;
            }
        }
    }
    catch (boost::system::system_error & se)
    {
        if (se.code() != http::error::end_of_stream )
            throw ;
    }

    // Send a TCP shutdown
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
    // we ignore the error because the client might have
    // dropped the connection already.
}

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
net::awaitable<void>
do_listen(
        tcp::endpoint endpoint)
{
    // Open the acceptor
    auto acceptor = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(tcp::acceptor(co_await net::this_coro::executor));
    acceptor.open(endpoint.protocol());

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true));

    // Bind to the server address
    acceptor.bind(endpoint);

    // Start listening for connections
    acceptor.listen(net::socket_base::max_listen_connections);

    INFO << "starting server";
    for(;;)
    {
        boost::asio::co_spawn(
                acceptor.get_executor(),
                do_session(tcp_stream(co_await acceptor.async_accept())),
                [](const std::exception_ptr& e)
                {
                    if (e)
                        try
                        {
                            std::rethrow_exception(e);
                        }
                        catch (std::exception &e) {
                            std::cerr << "Error in session: " << e.what() << "\n";
                        }
                });
    }

}

//------------------------------------------------------------------------------
