#include "auth_request_factory.h"

#include "chunk_body_sha256.h"
#include "raw_body.h"
#include "raw_body_sha256.h"

#include "common/telemetry/log.h"

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

namespace {

std::unique_ptr<ep::http::body> make_body(partial_parse_result& req,
                                          std::optional<auth_info> auth) {

    bool chunked =
        req.optional("content-encoding").value_or("none") == "aws-chunked" ||
        req.optional("transfer-encoding").value_or("none") == "chunked";

    if (chunked) {
        LOG_INFO() << req.peer << ": unauthenticated chunked transfer";
        return std::make_unique<chunked_body>(req);
    }

    auto length = std::stoul(req.optional("content-length").value_or("0"));

    if (!auth) {
        LOG_DEBUG() << req.peer << ": using single-chunk unauthenticated body";
        return std::make_unique<raw_body>(req, length);
    }

    auto content_sha = req.require("x-amz-content-sha256");

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256";
        return std::make_unique<chunk_body_sha256>(
            req, *auth, chunked_body::trailing_headers::none);
    }

    if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        LOG_DEBUG() << req.peer
                    << ": using chunked unsigned payload with trailer";
        return std::make_unique<chunked_body>(
            req, chunked_body::trailing_headers::read);
    }

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER") {
        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256 with trailer";
        return std::make_unique<chunk_body_sha256>(
            req, *auth, chunked_body::trailing_headers::read);
    }

    if (content_sha == "UNSIGNED-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using single-chunk unsigned body";
        return std::make_unique<raw_body>(req, length);
    }

    LOG_DEBUG() << req.peer << ": using single-chunk body with signed payload";
    return std::make_unique<raw_body_sha256>(req, *auth, length);
}

} // namespace

auth_request_factory::auth_request_factory(std::unique_ptr<user::backend> users)
    : m_users(std::move(users)) {}

coro<std::unique_ptr<request>>
auth_request_factory::create(ip::tcp::socket& sock) {

    auto req = co_await partial_parse_result::read(sock);
    auto auth = auth_info::create(req, *m_users);

    auto body = make_body(req, auth);
    auto rv = std::make_unique<request>(req, std::move(body));

    if (auth->authenticated_user) {
        rv->authenticated_user(std::move(*auth->authenticated_user));
    }

    co_return rv;
}

} // namespace uh::cluster::ep::http
