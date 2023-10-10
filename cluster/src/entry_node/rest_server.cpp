#include "rest_server.h"
#include "rest/utils/parser/s3_parser.h"
#include <fstream>
#include <filesystem>
#include "entry_node/rest/utils/string/string_utils.h"

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

//    coro<http::http_response>
//    rest_server::handle_requests (const http::http_request& req) const
//    {
//
//        http::http_response s3_res(req);
//        auto& underlying_res = s3_res.get_underlying_object();
//
//        if (req.get_request_name() == http::http_request_type::INIT_MULTIPART_UPLOAD)
//        {
//            underlying_res.set(boost::beast::http::field::transfer_encoding, "chunked");
//            underlying_res.set(boost::beast::http::field::connection, "keep-alive");
//            underlying_res.set(boost::beast::http::field::content_type, "application/xml");
//
//            // TODO: For now, we use fixed upload id for a client
//            underlying_res.body() =  std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
//                                      "<InitiateMultipartUploadResult>\n"
//                                      "<Bucket>myBucket</Bucket>\n"
//                                      "<Key>myObject</Key>\n"
//                                      "<UploadId>first-upload</UploadId>\n"
//                                      "</InitiateMultipartUploadResult>");
//
//        }
////        else if(req.get_request_name() == http::http_request_type::COMPLETE_MULTIPART_UPLOAD)
////        {
////            underlying_res.set(boost::beast::http::field::transfer_encoding, "chunked");
////            underlying_res.set(boost::beast::http::field::connection, "close");
////            underlying_res.set(boost::beast::http::field::content_type, "application/xml");
////            underlying_res.body() =  std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
////                                      "<CompleteMultipartUploadResult>\n"
////                                      "<Location>string</Location>\n"
////                                      "<Bucket>" + req.get_URI().get_bucket_id() +"</Bucket>\n"
////                                      "<Key>" + req.get_URI().get_object_key() + "</Key>\n"
////                                      "<ETag>string</ETag>\n"
////                                      "</CompleteMultipartUploadResult>");
////        }
////        else if (req.get_request_name() == http::http_request_type::PUT_OBJECT)
////        {
////            co_return co_await m_handler.handle(req, s3_res);
////        }
////        else if (req.get_request_name() == http::http_request_type::GET_OBJECT)
////        {
////            co_return co_await m_handler.handle(req, s3_res);
////        }
////        else if (req.get_request_name() == http::http_request_type::CREATE_BUCKET)
////        {
////            co_return co_await m_handler.handle(req, s3_res);
////        }
//
//        co_return std::move(s3_res);
//    }

//------------------------------------------------------------------------------

    net::awaitable<void>
    rest_server::do_session(tcp_stream stream) {
        std::cout << "connection from: " << stream.socket().remote_endpoint() << std::endl;

        beast::error_code ec;

        try {
            for (;;)
            {
                stream.expires_after(std::chrono::seconds(10000));
                beast::flat_buffer buffer;

                // read header
                b_http::request_parser<b_http::empty_body> received_request;
                received_request.body_limit((std::numeric_limits<std::uint64_t>::max)());

                co_await b_http::async_read_header(stream, buffer, received_request, net::use_awaitable);
                std::cout << received_request.get().base() << std::endl;

                // parse
                rest::utils::parser::s3_parser s3_parser(received_request, m_uomap_multipart);
                auto s3_request = s3_parser.parse();

                // read body
                co_await s3_request->read_body(stream, buffer);

                // authenticate

                // handle
                auto s3_res = co_await handle_requests(*s3_request);

                // send response
                co_await b_http::async_write(stream, s3_res.get_underlying_object(), net::use_awaitable);

                if(! received_request.keep_alive() )
                {
                    break;
                }
            }
        }
            // TODO: don't send all the info to the user on throw, and also we need to send appropriatre error code
            // like 401 on authorization failed and not 400 which is a bad request
        catch (boost::system::system_error &se)
        {
            if (se.code() != b_http::error::end_of_stream)
            {
                b_http::response<b_http::string_body> res{b_http::status::bad_request, 11};
                res.set(b_http::field::server, "UltiHash");
                res.set(b_http::field::content_type, "text/html");
                res.body() = se.code().message() + '\n';
                res.prepare_payload();
                b_http::write(stream, res);
                stream.socket().shutdown(tcp::socket::shutdown_send, ec);
                throw;
            }
        }
        catch (const std::exception& e)
        {
            b_http::response<b_http::string_body> res{b_http::status::bad_request, 11};
            res.set(b_http::field::server, "UltiHash");
            res.set(b_http::field::content_type, "text/html");
            res.body() = e.what();
            res.prepare_payload();
            b_http::write(stream, res);
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
