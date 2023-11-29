#include <common/log.h>
#include "rest_server.h"
#include "rest/utils/parser/s3_parser.h"
#include <fstream>
#include <filesystem>
#include "entry_node/rest/utils/string/string_utils.h"
#include "rest/http/models/custom_error_response_exception.h"

namespace uh::cluster::rest
{

//------------------------------------------------------------------------------

    rest_server::rest_server(entry_node_config config, std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes) :
        m_config(config), m_ioc(std::make_shared <boost::asio::io_context>(static_cast<int>(m_config.rest_server_conf.threads))),
        m_ssl(boost::asio::ssl::context::tlsv12_client),
        m_thread_container(m_config.rest_server_conf.threads-1),
        m_handler (dedupe_nodes, directory_nodes, m_config)
    {
        boost::asio::co_spawn(*m_ioc,
                              recover_failed_nodes (),
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

        boost::asio::co_spawn(*m_ioc,
                              do_listen(tcp::endpoint{m_server_address, m_config.rest_server_conf.port}),
                              [](const std::exception_ptr& e)
                              {
                                  if (e)
                                      try
                                      {
                                          std::rethrow_exception(e);
                                      }
                                      catch(std::exception & e)
                                      {
                                        LOG_ERROR() << "acceptor failed: " << e.what();
                                      }
                              });
    }

//------------------------------------------------------------------------------

    void
    rest_server::run()
    {
        LOG_INFO() << "starting rest server";

        for(auto i = 0 ; i < m_config.rest_server_conf.threads - 1 ; i++)
            m_thread_container.emplace_back(
                    [&]
                    {
                        m_ioc->run();
                    });

        m_ioc->run();
    }

//------------------------------------------------------------------------------

    coro <void> rest_server::recover_failed_nodes ()
    {
        auto m = m_handler.get_recovery_director().acquire_messenger();
        co_await m.get().send(RECOVER_REQ, {});
        const auto h = co_await m.get().recv_header();
        m_recover_response.set_value(h.type);
    }

    coro <void>
    rest_server::do_session(tcp_stream stream)
    {
        LOG_INFO() << "connection from: " << stream.socket().remote_endpoint();

        beast::error_code ec;

        try
        {
            for (;;)
            {
                stream.expires_after(std::chrono::seconds(10000));
                beast::flat_buffer buffer;

                b_http::request_parser<b_http::empty_body> received_request;
                received_request.body_limit((std::numeric_limits<std::uint64_t>::max)());

                // read header
                co_await b_http::async_read_header(stream, buffer, received_request, net::use_awaitable);
                LOG_INFO() << "received request: " << received_request.get().base();

                // parse
                rest::utils::parser::s3_parser s3_parser(received_request, m_server_state);
                auto s3_request = s3_parser.parse();

                // read body
                co_await s3_request->read_body(stream, buffer);

                // validate
                s3_request->validate_request_specific_criteria();
                auto s3_res = co_await m_handler.handle(*s3_request, m_server_state);

                // send response
                auto s3_res_specific_object = s3_res->get_response_specific_object();
                co_await b_http::async_write(stream, s3_res_specific_object, net::use_awaitable);
                LOG_INFO() << "sent response: " << s3_res_specific_object.base();

                if(!received_request.keep_alive() )
                {
                    break;
                }
            }
        }
        catch (const boost::system::system_error& se)
        {
            if (se.code() != b_http::error::end_of_stream)
            {
                LOG_ERROR() << se.what();
                uh::cluster::rest::http::model::custom_error_response_exception err(b_http::status::bad_request);
                b_http::write(stream, err.get_response_specific_object());
                stream.socket().shutdown(tcp::socket::shutdown_send, ec);
                throw;
            }
        }
        catch (uh::cluster::rest::http::model::custom_error_response_exception& res_exc)
        {
            LOG_ERROR() << res_exc.what();
            b_http::write(stream, res_exc.get_response_specific_object());
            stream.socket().shutdown(tcp::socket::shutdown_send, ec);
            throw;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR() << e.what();
            uh::cluster::rest::http::model::custom_error_response_exception err(b_http::status::internal_server_error);
            b_http::write(stream, err.get_response_specific_object());
            stream.socket().shutdown(tcp::socket::shutdown_send, ec);
            throw;
        }

        stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

//------------------------------------------------------------------------------

    coro <void>
    rest_server::do_listen(tcp::endpoint endpoint)
    {
        const auto recover_response = m_recover_response.get_future().get();
        if (recover_response != RECOVER_RESP) [[unlikely]]
        {
            throw std::runtime_error ("Recovery not successful!");
        }

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
                                LOG_ERROR() << "in session: [" << conn_address << ":" << conn_port << "] " << e.what();
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
