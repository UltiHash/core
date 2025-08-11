#pragma once

#include <gateway/command_factory.h>

#include <entrypoint/cors/module.h>
#include <entrypoint/http/request_factory.h>
#include <entrypoint/policy/module.h>

using namespace uh::cluster::ep;

namespace uh::cluster::gateway {

class handler_factory;

class handler : public protocol_handler {
public:
    handler(boost::asio::ip::tcp::socket&& client, handler_factory& factory)
        : m_client{std::move(client)},
          m_factory{factory} {
        LOG_INFO() << "session started: " << m_client.remote_endpoint();
    }

    coro<void> run() override;

private:
    boost::asio::ip::tcp::socket m_client;
    handler_factory& m_factory;

    coro<void> handle_request(boost::asio::ip::tcp::socket& endpoint,
                              const std::string& id);
};

class handler_factory : public protocol_handler_factory {
public:
    explicit handler_factory(command_factory&& comm_factory,
                             http::request_factory&& factory,
                             std::unique_ptr<policy::module> policy,
                             std::unique_ptr<cors::module> cors);

    std::unique_ptr<protocol_handler>
    create_handler(boost::asio::ip::tcp::socket&& s) {
        return std::make_unique<handler>(std::move(s), *this);
    }

private:
    friend class handler;
    command_factory m_command_factory;
    http::request_factory m_request_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;
};
} // namespace uh::cluster::gateway
