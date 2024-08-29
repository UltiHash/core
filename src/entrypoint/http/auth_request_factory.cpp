#include "auth_request_factory.h"

#include "beast_utils.h"
#include "chunk_body_sha256.h"
#include "raw_body.h"

#include "common/telemetry/log.h"

namespace uh::cluster::ep::http {

std::unique_ptr<http_request>
auth_request_factory::multi_chunk(partial_parse_result& req) {
    auto length = std::stoul(req.require("content-length"));

    // TODO: can there be chunked transfer without auth?
    if (!req.auth) {
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, length));
    }

    auto user = m_user_backend->find(req.auth->access_key_id);
    req.set_secret(user.secret_key);

    auto content_sha = req.require("x-amz-content-sha256");

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256";

        auto body = std::make_unique<chunk_body_sha256>(
            req, chunked_body::trailing_headers::none);

        return std::make_unique<http_request>(req, std::move(body));
    }

    if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        LOG_DEBUG() << req.peer
                    << ": using chunked unsigned payload with trailer";

        return std::make_unique<http_request>(
            req, std::make_unique<chunked_body>(
                     req, chunked_body::trailing_headers::read));
    }

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER") {
        auto body = std::make_unique<chunk_body_sha256>(
            req, chunked_body::trailing_headers::read);

        return std::make_unique<http_request>(req, std::move(body));
    }

    throw std::runtime_error("unsupported aws-chunked authentication: " +
                             content_sha);
}

std::unique_ptr<http_request>
auth_request_factory::single_chunk(partial_parse_result& req) {

    auto length = std::stoul(req.optional("content-length").value_or("0"));

    if (!req.auth) {
        LOG_DEBUG() << req.peer << ": using single-chunk unauthenticated body";
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, length));
    }

    auto user = m_user_backend->find(req.auth->access_key_id);
    req.set_secret(user.secret_key);

    auto content_sha = req.require("x-amz-content-sha256");
    if (content_sha == "UNSIGNED-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using single-chunk unsigned body";
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, length));
    }

    // TODO insert check for payload signature
    LOG_DEBUG() << req.peer << ": using single-chunk body with signed payload";
    return std::make_unique<http_request>(
        req, std::make_unique<raw_body>(req, length));
}

auth_request_factory::auth_request_factory(
    std::unique_ptr<user::backend> user_backend)
    : m_user_backend(std::move(user_backend)) {}

coro<std::unique_ptr<http_request>>
auth_request_factory::create(boost::asio::ip::tcp::socket& sock) {
    auto req = co_await partial_parse_result::read(sock);

    LOG_DEBUG() << req.peer << ": pre-auth request: " << req.headers;

    auto transfer_encoding = req.optional("content-encoding");
    if (transfer_encoding && *transfer_encoding == "aws-chunked") {
        co_return multi_chunk(req);
    }

    co_return single_chunk(req);
}

} // namespace uh::cluster::ep::http
