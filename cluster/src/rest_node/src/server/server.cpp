#include <mpi.h>
#include "server.h"
#include "s3_parser.h"
#include "functions.h"

namespace uh::rest
{

//------------------------------------------------------------------------------

    rest_server::rest_server(uh::rest::rest_server_config&& config, const uh::cluster::cluster_ranks& cluster_plan) :
        m_config(std::move(config)), m_ioc(static_cast<int>(m_config.threads)), m_thread_container(m_config.threads-1),
        m_cluster_plan(cluster_plan)
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
        beast::error_code ec;

        std::unordered_map<req_types, std::function<void(const s3_parser<true>&)>> request_to_function
            { { put_object, [this](const s3_parser<true>& s3_parser) { putObject(s3_parser, m_cluster_plan); } },
              { get_object, [this](const s3_parser<true>& s3_parser) { getObject(s3_parser, m_cluster_plan); } } };

        try
        {
            for(;;)
            {
                stream.expires_after(std::chrono::seconds(10));

                // Read a request
                uh::rest::s3_parser<true> s3_parser;
                s3_parser.body_limit(1024ul*1024ul*1024ul);

                co_await http::async_read(stream, buffer, s3_parser, net::use_awaitable);

//                // TODO: co await mechanism send mpi
//                // TODO: use mpi scatter to send to a specific communicator processes
                request_to_function[s3_parser.m_parsed_struct.req_type](s3_parser);

            }
        }
        catch (boost::system::system_error & se)
        {
            if (se.code() != http::error::end_of_stream )
            {
                // Send a response about the error and shut down
                http::response<http::string_body> res{http::status::bad_request, 11};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.body() = se.code().message() + '\n';
                res.prepare_payload();

                // TODO: Should this write also be async? If so how to use it with coroutine
                http::write(stream, res);
                stream.socket().shutdown(tcp::socket::shutdown_send, ec);
                throw ;
            }

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
                                INFO << "Error in session: [" << conn_address << ":" << conn_port << "] " << e.what();
                            }
                    });
        }

    }

//------------------------------------------------------------------------------

} // namespace uh::rest
