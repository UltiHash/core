
#include "rest_server.h"
#include "s3_parser.h"
#include "s3_authenticator.h"
#include <fstream>

namespace uh::cluster
{

//------------------------------------------------------------------------------

    rest_server::rest_server(server_config config, std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes) :
        m_config(config), m_ioc(std::make_shared <boost::asio::io_context>(static_cast<int>(m_config.threads))),
        m_ssl(boost::asio::ssl::context::tlsv12_client),
        m_thread_container(m_config.threads-1),
        m_handler (dedupe_nodes, directory_nodes)
    {
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
                std::string body_buffer;
                received_request.body_limit((std::numeric_limits<std::uint64_t>::max)());

                // read header first
                co_await http::async_read_header(stream, buffer, received_request, net::use_awaitable);
                std::cout << received_request.get().base() << std::endl;

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

                // Determine the content length from the header
                if (received_request.get().find(http::field::content_length) != received_request.get().end())
                {
                    std::size_t content_length = std::stoull(received_request.get()[http::field::content_length]);

                    body_buffer.append(content_length, 0);
                    auto data_left = content_length - buffer.size();

                    // copy remaining bytes from flat buffer to body_buffer
                    boost::asio::buffer_copy(boost::asio::buffer(body_buffer), buffer.data());
                    auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer(body_buffer.data() + buffer.size(), data_left),
                                                                             boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                    if (size_transferred + buffer.size() != content_length)
                    {
                        throw std::runtime_error("error reading the http body");
                    }
                }
                else
                {
                    throw std::runtime_error("please specify the content length");
                }

                // check for invalid header
                s3_parser s3_parser(received_request);
                auto parsed_request = s3_parser.parse();
                parsed_request.body = body_buffer;

//                s3_authenticator s3_authenticate(received_request, parsed_request);
//                s3_authenticate.authenticate();

                auto response = co_await m_handler.handle(parsed_request);

//                // send response
                co_await http::async_write(stream, response, net::use_awaitable);

                if(! received_request.keep_alive() )
                {
                    break;
                }
            }
        }
            // TODO: don't send all the info to the user on throw
        catch (boost::system::system_error &se)
        {
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
    }

//------------------------------------------------------------------------------

    net::awaitable<void>
    rest_server::do_listen(tcp::endpoint endpoint)
    {
        // TODO : implement ssl
//        m_ssl.use_certificate_chain_file(config.tls_chain);
//        m_ssl.use_private_key_file(config.tls_pkey, ssl::context::pem);

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
        for (auto& thread: m_thread_container)
        {
            thread.join();
        }
    }

//------------------------------------------------------------------------------

} // namespace uh::cluster
