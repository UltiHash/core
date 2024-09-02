#ifndef CORE_ENTRYPOINT_HTTP_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_REQUEST_FACTORY_H

#include "beast_utils.h"
#include "body.h"
#include "http_request.h"

#include <memory>

namespace uh::cluster::ep::http {

class body_factory {
public:
    virtual ~body_factory() = default;
    virtual std::unique_ptr<body> create(partial_parse_result& body) = 0;
};

class request_factory {
public:
    request_factory(std::unique_ptr<body_factory> body);

    coro<std::unique_ptr<http_request>> create(boost::asio::ip::tcp::socket&);

private:
    std::unique_ptr<body_factory> m_body;
};

} // namespace uh::cluster::ep::http

#endif
