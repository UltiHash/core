#ifndef CORE_ENTRYPOINT_HTTP_AUTH_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_AUTH_REQUEST_FACTORY_H

#include "request_factory.h"

namespace uh::cluster::ep::http {

class auth_request_factory : public request_factory {
public:
    auth_request_factory(std::unique_ptr<request_factory> base);

    coro<std::unique_ptr<http_request>>
    create(boost::asio::ip::tcp::socket&) override;

private:
    std::unique_ptr<request_factory> m_base;
};

} // namespace uh::cluster::ep::http

#endif
