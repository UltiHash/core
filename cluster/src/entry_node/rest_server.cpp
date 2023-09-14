
#include "rest_server.h"
#include "s3_parser.h"
#include "s3_authenticator.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

    rest_server::rest_server(server_config config, std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes) :
        m_config(config), m_ioc(std::make_shared <boost::asio::io_context>(static_cast<int>(m_config.threads))),
        m_thread_container(m_config.threads-1),
        m_handler (dedupe_nodes, directory_nodes)
    {
        // spawn a coroutine
        boost::asio::co_spawn(*m_ioc,
                              do_listen(tcp::endpoint{m_server_address, m_config.port}),
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

    void
    rest_server::run()
    {
        std::cout << "starting server" << std::endl;

        for(auto i = 0 ; i < m_config.threads - 1 ; i++)
            m_thread_container.emplace_back(
                    [&]
                    {
                        m_ioc->run();
                    });

        m_ioc->run();
    }

//------------------------------------------------------------------------------

    net::awaitable<void>
    rest_server::do_session(tcp_stream stream) {
        std::cout << "connection from: " << stream.socket().remote_endpoint() << std::endl;

        // This buffer is required to persist across reads
        beast::flat_buffer buffer;
        beast::error_code ec;

        try {
            for (;;) {
                stream.expires_after(std::chrono::seconds(10000));

                http::request_parser<http::empty_body> received_request;
                received_request.body_limit((std::numeric_limits<std::uint64_t>::max)());

                // read header first
                co_await http::async_read_header(stream, buffer, received_request, net::use_awaitable);

                // expect-100-continue
                if (received_request.get()[http::field::expect] == "100-continue")
                {
                    http::response<http::empty_body> res;
                    res.version(11);

                    if (!m_server_busy)
                        res.result(http::status::continue_);
                    else
                        res.result(http::status::payload_too_large);

                    co_await http::async_write(stream, res, net::use_awaitable);
                }

//                // Process body in chunks
//                std::vector<char> chunk(450029190); // Adjust the chunk size as needed
//                while ()
//                {
//                    std::size_t bytes_transferred = co_await stream.async_read_some(net::buffer(chunk), net::use_awaitable);
//                    std::cout << bytes_transferred << std::endl;
//                    if (bytes_transferred == 0)
//                        break; // End of body
//
//                }

//                std::cout << std::string{chunk.data(), chunk.size()} << std::endl;

//                // check for invalid headers before getting the body
//                s3_parser s3_parser(received_request);
//                auto parsed_request = s3_parser.parse();
//
//                co_await http::async_read(stream, buffer, received_request, net::use_awaitable);
//                std::cout << received_request.get().body().size() << std::endl;

//                s3_authenticator s3_authenticate(received_request, parsed_request);
//                s3_authenticate.authenticate();

//                auto response = co_await m_handler.handle(parsed_request);

                // send response
//                co_await http::async_write(stream, response, net::use_awaitable);

                if(! received_request.keep_alive() )
                {
                    break;
                }
            }
        }
        catch (boost::system::system_error &se) {
            if (se.code() != http::error::end_of_stream)
            {
                http::response<http::string_body> res{http::status::bad_request, 11};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.body() = se.code().message() + '\n';
                res.prepare_payload();
                http::write(stream, res);
                stream.socket().shutdown(tcp::socket::shutdown_send, ec);
                throw;
            }
        }
        catch (const std::exception& e)
        {
            http::response<http::string_body> res{http::status::bad_request, 11};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.body() = e.what();
            res.prepare_payload();
            http::write(stream, res);
            stream.socket().shutdown(tcp::socket::shutdown_send, ec);
            throw;
        }

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
            auto stream = tcp_stream(co_await acceptor.async_accept());
            auto conn_address = stream.socket().remote_endpoint().address().to_string();
            auto conn_port = stream.socket().remote_endpoint().port();

            boost::asio::co_spawn(
                    acceptor.get_executor(),
                    do_session(std::move(stream)),
                    [&](const std::exception_ptr& e)
                    {
                        if (e)
                            try
                            {
                                std::rethrow_exception(e);
                            }
                            catch (const std::exception &e)
                            {
                                std::cout << "Error in session: [" << conn_address << ":" << conn_port << "] " << e.what() << "\n";
                            }
                    });
        }

    }

//------------------------------------------------------------------------------

    std::shared_ptr<boost::asio::io_context> rest_server::get_executor() const
    {
        return m_ioc;
    }

//------------------------------------------------------------------------------

    rest_server::~rest_server() {
        for (auto& thread: m_thread_container) {
            thread.join();
        }
    }

//------------------------------------------------------------------------------

} // namespace uh::cluster
