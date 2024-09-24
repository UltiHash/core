#include "request_factory.h"

#include "aws4_hmac_sha256.h"

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

request_factory::request_factory(user::db&& users)
    : m_users(std::move(users)) {}

coro<std::unique_ptr<request>> request_factory::create(ip::tcp::socket& sock) {
    auto req = co_await partial_parse_result::read(sock);

    auto authorization = req.optional("Authorization");
    if (authorization && authorization->starts_with("AWS4_HMAC_SHA256")) {
        co_return co_await aws4_hmac_sha256::create(m_users, req);
    }

    throw std::runtime_error("unknown authentication scheme");
}

} // namespace uh::cluster::ep::http
