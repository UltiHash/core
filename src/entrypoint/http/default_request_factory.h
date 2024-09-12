#ifndef CORE_ENTRYPOINT_HTTP_DEFAULT_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_DEFAULT_REQUEST_FACTORY_H

#include "request_factory.h"

namespace uh::cluster::ep::http {

class default_request_factory : public request_factory {
public:
    coro<std::unique_ptr<request>>
    create(boost::asio::ip::tcp::socket&) override;
};

} // namespace uh::cluster::ep::http

#endif
