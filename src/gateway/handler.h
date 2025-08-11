#pragma once

#include <gateway/command_factory.h>

#include <entrypoint/cors/module.h>
#include <entrypoint/http/request_factory.h>
#include <entrypoint/policy/module.h>

using namespace uh::cluster::ep;

namespace uh::cluster::gateway {

class handler : public protocol_handler {
public:
    explicit handler(command_factory&& comm_factory,
                     http::request_factory&& factory,
                     std::unique_ptr<policy::module> policy,
                     std::unique_ptr<cors::module> cors);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    command_factory m_command_factory;
    http::request_factory m_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;

    coro<void> handle_request(boost::asio::ip::tcp::socket& s,
                              boost::asio::ip::tcp::socket& endpoint,
                              const std::string& id);
};

} // namespace uh::cluster::gateway
