#include "request_factory.h"

#include "aws4_hmac_sha256.h"
#include "basic_auth.h"
#include "no_auth.h"

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

request_factory::request_factory(user::db& users)
    : m_users(users) {}

coro<std::unique_ptr<request>> request_factory::create(ip::tcp::socket& sock) {
    auto req = co_await partial_parse_result::read(sock);

    LOG_DEBUG() << "pre-auth request: " << req.headers;

    auto authorization = req.optional("Authorization");
    if (authorization) {

        if (authorization->starts_with("AWS4-HMAC-SHA256 ")) {
            co_return co_await aws4_hmac_sha256::create(m_users, req);
        }

        if (authorization->starts_with("Basic ")) {
            co_return co_await basic_auth::create(m_users, req);
        }
    }

    auto url = parse_request_target(req.headers.target());
    if (auto key = url.params.find("X-Amz-Algorithm");
        key != url.params.end()) {
        LOG_DEBUG() << sock.remote_endpoint() << ": algorithm: " << key->second;
        co_return co_await aws4_hmac_sha256::create_from_url(m_users, req);
    }

    co_return co_await no_auth::create(req);
}

} // namespace uh::cluster::ep::http
