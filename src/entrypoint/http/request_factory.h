#ifndef CORE_ENTRYPOINT_HTTP_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_REQUEST_FACTORY_H

#include "beast_utils.h"
#include "body.h"
#include "request.h"

#include <memory>

namespace uh::cluster::ep::http {

class request_factory {
public:
    virtual ~request_factory() = default;
    virtual coro<std::unique_ptr<request>>
    create(boost::asio::ip::tcp::socket&) = 0;
};

} // namespace uh::cluster::ep::http

#endif
