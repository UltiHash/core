#include "request_factory.h"

#include "common/telemetry/log.h"

namespace uh::cluster::ep::http {

request_factory::request_factory(std::unique_ptr<body_factory> body)
    : m_body(std::move(body)) {}

coro<std::unique_ptr<http_request>>
request_factory::create(boost::asio::ip::tcp::socket& sock) {
    auto req = co_await partial_parse_result::read(sock);
    LOG_DEBUG() << req.peer << ": pre-auth request: " << req.headers;

    co_return std::make_unique<http_request>(req, m_body->create(req));
}

} // namespace uh::cluster::ep::http
