
#include "rest_server.h"
#include "rest/utils/parser/s3_parser.h"
#include <fstream>
#include <filesystem>

namespace uh::cluster
{

//------------------------------------------------------------------------------
    template <typename T, typename Y>
    void ts_map<T,Y>::insert(const T& key, const Y& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container[key] = value;
    }

//------------------------------------------------------------------------------

    template <typename T, typename Y>
    std::map<T,Y>::iterator ts_map<T,Y>::find(const T& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container.find(key);
    }

//------------------------------------------------------------------------------

    template <typename T, typename Y>
    Y& ts_map<T,Y>::operator[] (const T& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container[key];
    }

//------------------------------------------------------------------------------

    template <typename T, typename Y>
    std::map<T, Y>::iterator ts_map<T,Y>::begin()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container.begin();
    }

//------------------------------------------------------------------------------

    template <typename T, typename Y>
    std::map<T, Y>::iterator ts_map<T,Y>::end()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container.end();
    }

//------------------------------------------------------------------------------
    template <typename T, typename Y>
    void ts_map<T, Y>::clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.clear();
    }

//------------------------------------------------------------------------------

    rest_server::rest_server(server_config config, std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes) :
        m_config(config), m_ioc(std::make_shared <boost::asio::io_context>(static_cast<int>(m_config.threads))),
        m_ssl(boost::asio::ssl::context::tlsv12_client),
        m_thread_container(m_config.threads-1)
//        m_handler (dedupe_nodes, directory_nodes)
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

//    coro<http::response<http::string_body>>
//    rest_server::handle_requests (parsed_request_wrapper& parsed_request, tcp_stream& stream,
//                                  beast::flat_buffer& remaining_buffer)
//    {
//        std::string body_buffer;
//
//        // common response headers
//        http::response<http::string_body> res {http::status::ok, 11};
//
//        if (parsed_request.req_type == init_multi_part)
//        {
//            res.set(boost::beast::http::field::transfer_encoding, "chunked");
//            res.set(boost::beast::http::field::connection, "keep-alive");
//            res.set(boost::beast::http::field::content_type, "application/xml");
//
//            // TODO: For now, we use fixed upload id for a client
//            res.body() =  std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
//                                      "<InitiateMultipartUploadResult>\n"
//                                      "<Bucket>" + parsed_request.bucket_id + "</Bucket>\n"
//                                      "<Key>" + parsed_request.object_key + "</Key>\n"
//                                      "<UploadId>1</UploadId>\n"
//                                      "</InitiateMultipartUploadResult>");
//
//            co_return std::move(res);
//        }
//
//        else if (parsed_request.req_type == close_multi_part)
//        {
//            if (!m_is_close)
//            {
//                m_is_close = true;
//                for (const auto& pair : m_multi_part_container)
//                {
//                    body_buffer += pair.second;
//                }
//
//                parsed_request.body = body_buffer;
//                co_return co_await m_handler.handle(parsed_request);
//            }
//        }
//
//        else if (parsed_request.req_type == delete_multi_part_upload)
//        {
//            m_multi_part_container.clear();
//            res.prepare_payload();
//            co_return std::move(res);
//        }
//
//        else if (parsed_request.req_type == get_object)
//        {
//            co_return co_await m_handler.handle(parsed_request);
//        }
//
//        else if (parsed_request.req_type == put_object || parsed_request.req_type == multi_part_upload)
//        {
//            // expect-100-continue
//            if (parsed_request.http_parsed_fields[http_fields::expect] == "100-continue")
//            {
//                http::response<http::string_body> res_expect;
//                res_expect.set(boost::beast::http::field::version, "11");
//
//                if (!m_server_busy)
//                    res_expect.result(http::status::continue_);
//                else
//                    res_expect.result(http::status::payload_too_large);
//
//                co_await http::async_write(stream, res_expect, net::use_awaitable);
//            }
//
//            if (parsed_request.http_parsed_fields.contains(http_fields::content_length) &&
//                !parsed_request.http_parsed_fields[http_fields::content_length].empty())
//            {
//                std::size_t content_length = std::stoull(static_cast<const std::string>(parsed_request.http_parsed_fields[http_fields::content_length]));
//                auto data_left = content_length - remaining_buffer.size();
//
//
//                if (parsed_request.req_type == multi_part_upload)
//                {
//
//                    if ( m_multi_part_container.find(parsed_request.part_number) == m_multi_part_container.end() )
//                    {
//                        m_multi_part_container[parsed_request.part_number] = "";
//                        auto& multipart_buffer = m_multi_part_container[parsed_request.part_number];
//                        multipart_buffer.append(content_length, 0);
//                        boost::asio::buffer_copy(boost::asio::buffer(multipart_buffer), remaining_buffer.data());
//
//                        auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer(multipart_buffer.data() + remaining_buffer.size(), data_left),
//                                                                                 boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);
//
//                        if (size_transferred + remaining_buffer.size() != content_length)
//                        {
//                            throw std::runtime_error("error reading the http body");
//                        }
//
//                        res.set(http::field::etag, "ThisistheCustomEtag" + std::to_string(parsed_request.part_number));
//                        res.prepare_payload();
//
//                        co_return std::move(res);
//                    }
//                }
//                else
//                {
//                    body_buffer.append(content_length, 0);
//
//                    // copy remaining bytes from flat buffer to body_buffer
//                    boost::asio::buffer_copy(boost::asio::buffer(body_buffer), remaining_buffer.data());
//                    auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer(body_buffer.data() + remaining_buffer.size(), data_left),
//                                                                             boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);
//
//                    if (size_transferred + remaining_buffer.size() != content_length)
//                    {
//                        throw std::runtime_error("error reading the http body");
//                    }
//
//                    parsed_request.body = body_buffer;
//
//                    co_return co_await m_handler.handle(parsed_request);
//                }
//            }
//            else
//            {
//                throw std::runtime_error("please specify the content length on requests as other methods are currently not supported");
//            }
//        }
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
                http::request_parser<http::empty_body> received_request;
                received_request.body_limit((std::numeric_limits<std::uint64_t>::max)());

                co_await http::async_read_header(stream, buffer, received_request, net::use_awaitable);
                std::cout << received_request.get().base() << std::endl;
                // parse
                rest::utils::parser::s3_parser s3_parser(received_request);
                auto parsed_service_ptr = s3_parser.parse();

                for (const auto& pair : parsed_service_ptr->get_headers())
                    std::cout << pair.first << " : " << pair.second << std::endl;

//                // handle
//                auto res = co_await handle_requests(parsed_request, stream, buffer);
//
//                // send response
//                co_await http::async_write(stream, res, net::use_awaitable);
//
//                // TODO: find a way to remove this
//                if (parsed_request.req_type == close_multi_part)
//                    m_is_close = false;

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
            if (se.code() != http::error::end_of_stream)
            {
                http::response<http::string_body> res{http::status::bad_request, 11};
                res.set(http::field::server, "UltiHash");
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
            res.set(http::field::server, "UltiHash");
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
