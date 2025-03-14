#pragma once

#include "command_factory.h"
#include "cors/module.h"
#include "http/request_factory.h"
#include "policy/module.h"

namespace uh::cluster::ep {

class handler : public protocol_handler {
public:
    explicit handler(command_factory&& comm_factory,
                     http::request_factory&& factory,
                     std::unique_ptr<policy::module> policy,
                     std::unique_ptr<cors::module> cors);

    notrace_coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    command_factory m_command_factory;
    http::request_factory m_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;

    enum class flow_control : uint8_t { BREAK, CONTINUE };
    coro<flow_control> handle_request(boost::asio::ip::tcp::socket& s,
                                      http::raw_request& req,
                                      const std::string& id);

    coro<http::response> execute_request(boost::asio::ip::tcp::socket& s,
                                         http::request& req,
                                         const std::string& id);
};

} // end namespace uh::cluster::ep
