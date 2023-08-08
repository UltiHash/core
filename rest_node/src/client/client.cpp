#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

using tcp_stream = typename beast::tcp_stream::rebind_executor<
        net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
net::awaitable<void>
do_session(
        std::string host,
        std::string port,
        std::string target,
        int version)
{
    // These objects perform our I/O
    auto resolver = net::use_awaitable.as_default_on(tcp::resolver(co_await net::this_coro::executor));
    auto stream = tcp_stream(co_await net::this_coro::executor);

    // Look up the domain name
    auto const results = co_await resolver.async_resolve(host, port);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(10));

    // Make the connection on the IP address we get from a lookup
    co_await stream.async_connect(results);

    for(;;)
    {
        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        co_await http::async_write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer b;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        co_await http::async_read(stream, b, res);

        // Write the message to standard out
        std::cout << "Thread ID:" << std::this_thread::get_id() << "\n" << res << '\n' << std::endl;
    }
}

int main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc != 4)
    {
        std::cerr <<
                  "Usage: http-client-awaitable <host> <port> <target>\n" <<
                  "Example:\n" <<
                  "    http-client-awaitable www.example.com 80 5 /\n";
        return EXIT_FAILURE;
    }

    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];
    auto const threads = 5;
    int version = 11;

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    for (int i=0; i < 5 ; i++)
    {
        net::co_spawn(ioc,
                      do_session(host, port, target, version),
                      [](const std::exception_ptr& e)
                      {
                          if (e)
                              try
                              {
                                  std::rethrow_exception(e);
                              }
                              catch(std::exception & e)
                              {
                                  std::cerr << "Error: " << e.what() << "\n";
                              }
                      });
    }

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
                [&ioc]
                {
                    ioc.run();
                });

    ioc.run();

    return EXIT_SUCCESS;
}