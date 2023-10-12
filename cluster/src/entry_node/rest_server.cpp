#include "rest_server.h"
#include "rest/utils/parser/s3_parser.h"
#include <fstream>
#include <filesystem>
#include "entry_node/rest/utils/string/string_utils.h"
#include "rest/http/models/generic_error_response.h"

namespace uh::cluster::rest
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

        beast::error_code ec;

        try
        {
            for (;;)
            {
                stream.expires_after(std::chrono::seconds(10000));
                beast::flat_buffer buffer;

                // read header
                b_http::request_parser<b_http::empty_body> received_request;
                received_request.body_limit((std::numeric_limits<std::uint64_t>::max)());

                co_await b_http::async_read_header(stream, buffer, received_request, net::use_awaitable);
                // log this to debug
                std::cout << received_request.get().base() << std::endl;

                // parse
                rest::utils::parser::s3_parser s3_parser(received_request, m_uomap_multipart);
                auto s3_request = s3_parser.parse();

                // read body
                co_await s3_request->read_body(stream, buffer);

                // authenticate

                // handle
                auto s3_res = co_await m_handler.handle(*s3_request);

                // send response
                const auto& res_to_send = s3_res->get_response_specific_object();
                co_await b_http::async_write(stream, res_to_send, net::use_awaitable);

                if(! received_request.keep_alive() )
                {
                    break;
                }
            }
        }
        catch (boost::system::system_error &se)
        {
            if (se.code() != b_http::error::end_of_stream)
            {
                uh::cluster::rest::http::model::error_response err;
                b_http::write(stream, err.get_response_specific_object());
                stream.socket().shutdown(tcp::socket::shutdown_send, ec);
                throw;
            }
        }
        catch (const std::exception& e)
        {
            uh::cluster::rest::http::model::error_response err;
            b_http::write(stream, err.get_response_specific_object());
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
