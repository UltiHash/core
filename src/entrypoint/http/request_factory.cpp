#include "request_factory.h"

#include "aws4_hmac_sha256.h"
#include "basic_auth.h"
#include "no_auth.h"

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

request_factory::request_factory(user::db& users)
    : m_users(users) {}

coro<std::unique_ptr<request>> request_factory::create(ip::tcp::socket& sock,
                                                       raw_request&& rawreq) {
    if (auto auth = rawreq.optional("Authorization"); auth) {

        if (auth->starts_with("AWS4-HMAC-SHA256 ")) {
            co_return co_await aws4_hmac_sha256::create(
                sock, m_users, std::move(rawreq), *auth);
        }

        if (auth->starts_with("Basic ")) {
            co_return co_await basic_auth::create(sock, m_users,
                                                  std::move(rawreq));
        }
    }

    if (auto key = rawreq.params.find("X-Amz-Algorithm");
        key != rawreq.params.end()) {
        LOG_DEBUG() << sock.remote_endpoint() << ": algorithm: " << key->second;
        co_return co_await aws4_hmac_sha256::create_from_url(sock, m_users,
                                                             std::move(rawreq));
    }

    co_return co_await no_auth::create(sock, std::move(rawreq));
}

} // namespace uh::cluster::ep::http
