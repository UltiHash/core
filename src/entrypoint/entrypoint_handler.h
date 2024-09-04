#ifndef ENTRYPOINT_HANDLER_H
#define ENTRYPOINT_HANDLER_H

#include "command_factory.h"
#include "http/request_factory.h"
#include "policy/module.h"

namespace uh::cluster {

class entrypoint_handler : public protocol_handler {
public:
    explicit entrypoint_handler(
        command_factory&& comm_factory,
        std::unique_ptr<ep::http::request_factory> factory,
        std::unique_ptr<ep::policy::module> policy);

    coro<void> on_startup() override;

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

    coro<http_response> handle_request(boost::asio::ip::tcp::socket& s,
                                       http_request& req) const;

private:
    command_factory m_command_factory;
    std::unique_ptr<ep::http::request_factory> m_factory;
    std::unique_ptr<ep::policy::module> m_policy;
};

} // end namespace uh::cluster

#endif // ENTRYPOINT_HANDLER_H
