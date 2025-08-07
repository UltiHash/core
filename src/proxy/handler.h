#pragma once

#include <proxy/command_factory.h>

#include <entrypoint/cors/module.h>
#include <entrypoint/http/request_factory.h>
#include <entrypoint/policy/module.h>

using namespace uh::cluster::ep;

namespace uh::cluster::proxy {

class handler_factory;

class handler : public protocol_handler {
public:
    handler(boost::asio::ip::tcp::socket&& client,
            boost::asio::ip::tcp::socket&& endpoint, handler_factory& factory)
        : m_client{std::move(client)},
          m_endpoint{std::move(endpoint)},
          m_factory{factory} {
        LOG_INFO() << "session started: " << m_client.remote_endpoint();
    }

    coro<void> run() override;

private:
    boost::asio::ip::tcp::socket m_client;
    boost::asio::ip::tcp::socket m_endpoint;
    handler_factory& m_factory;

    coro<void> handle_request(http::raw_request& rawreq, const std::string& id);
    coro<void> proxy_raw_request(http::raw_request& rawreq);
};

class handler_factory : public protocol_handler_factory {
public:
    explicit handler_factory(command_factory&& comm_factory,
                             http::request_factory&& factory,
                             std::unique_ptr<policy::module> policy,
                             std::unique_ptr<cors::module> cors);

    coro<std::unique_ptr<protocol_handler>>
    create_handler(boost::asio::ip::tcp::socket&& s) {

        std::string endpoint_host = "localhost";
        std::string endpoint_port = "8080";
        boost::asio::ip::tcp::resolver resolver(s.get_executor());
        auto endpoints = co_await resolver.async_resolve(
            endpoint_host, endpoint_port, boost::asio::use_awaitable);
        boost::asio::ip::tcp::socket endpoint_socket(s.get_executor());
        LOG_DEBUG() << "Connecting to endpoint " << endpoint_host << ":"
                    << endpoint_port;
        co_await boost::asio::async_connect(endpoint_socket, endpoints,
                                            boost::asio::use_awaitable);

        co_return std::make_unique<handler>(std::move(s),
                                            std::move(endpoint_socket), *this);
    }

private:
    friend class handler;
    command_factory m_command_factory;
    http::request_factory m_request_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;
};
} // namespace uh::cluster::proxy
