#include "server.h"
#include "s3_parser.h"

namespace uh::rest
{

//------------------------------------------------------------------------------

    rest_server::rest_server(const uh::rest::rest_server_config &config) :
        m_config(config), m_ioc(static_cast<int>(m_config.threads)), m_thread_container(m_config.threads-1)
    {
        // spawn a coroutine
        boost::asio::co_spawn(m_ioc,
                              do_listen(tcp::endpoint{m_config.address, m_config.port}),
                              [](const std::exception_ptr& e)
                              {
                                  if (e)
                                      try
                                      {
                                          std::rethrow_exception(e);
                                      }
                                      catch(std::exception & e)
                                      {
                                          std::cerr << "Error in acceptor: " << e.what() << "\n";
                                      }
                              });
    }

//------------------------------------------------------------------------------

    bool
    rest_server::is_busy() const
    {
        //TODO: implement busyness logic
        return false;
    }

//------------------------------------------------------------------------------

    void
    rest_server::run()
    {
        INFO << "starting server";

        for(auto i = 0 ; i < m_config.threads - 1 ; i++)
            m_thread_container.emplace_back(
                    [&]
                    {
                        m_ioc.run();
                    });

        // the calling thread is also running the I/O service
        m_ioc.run();
    }

//------------------------------------------------------------------------------

    template <class Body, class Allocator>
    http::response<http::string_body>
    rest_server::handle_request(http::request<Body, http::basic_fields<Allocator>> &&req)
        {
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

    net::awaitable<void>
    rest_server::do_session(tcp_stream stream)
    {
        INFO << "connection from: " << stream.socket().remote_endpoint();

        // This buffer is required to persist across reads
        beast::flat_buffer buffer;

        try
        {
            for(;;)
            {
                stream.expires_after(std::chrono::seconds(10));

                // Read a request
                uh::rest::s3_parser<true> custom_parser;
                co_await http::async_read(stream, buffer, custom_parser, net::use_awaitable);
                message = constrct_message (custom_parser.parsed_request);
                if (message is put object) {
                    send (message) to dedupe mpi tags mpi tag::dedupe
                }
                else if (message is get objecyt) {
                    send (message) to phonebook with mpi tag::read
                }
                ..



                get respnse
//                http::response<http::string_body> msg =
//                        handle_request(std::move(req));

//                bool keep_alive = msg.keep_alive();
//
//                // Send the response
//                co_await http::async_write(stream, std::move(msg), net::use_awaitable);

//                if(! keep_alive)
//                {
//                    break;
//                }
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
        // we do not handle any error code because at this point the connection is closed
    }

//------------------------------------------------------------------------------

    net::awaitable<void>
    rest_server::do_listen(tcp::endpoint endpoint)
    {
        auto acceptor = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(tcp::acceptor(co_await net::this_coro::executor));

        acceptor.open(endpoint.protocol());
        acceptor.set_option(net::socket_base::reuse_address(true));

        acceptor.bind(endpoint);
        acceptor.listen(net::socket_base::max_listen_connections);

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

} // namespace uh::rest
