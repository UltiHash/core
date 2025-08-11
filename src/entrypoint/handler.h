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

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    command_factory m_command_factory;
    http::request_factory m_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;

    coro<http::response> handle_request(boost::asio::ip::tcp::socket& s,
                                        http::raw_request& rawreq,
                                        const std::string& id);
};

} // end namespace uh::cluster::ep
