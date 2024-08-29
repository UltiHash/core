#include "auth_request_factory.h"

#include "beast_utils.h"
#include "chunk_body_sha256.h"
#include "raw_body.h"

#include "common/telemetry/log.h"

#include <boost/algorithm/string.hpp>
#include <set>

using namespace boost;

namespace uh::cluster::ep::http {

constexpr const char* SECRET_ACCESS_KEY = "secret";

std::unique_ptr<http_request> multi_chunk_request(partial_parse_result& req) {
    auto content_length = std::stoul(req.require("content-length"));

    // TODO: can there be chunked transfer without auth?
    if (!req.auth) {
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, content_length));
    }

    req.set_secret(SECRET_ACCESS_KEY);

    auto content_sha = req.require("x-amz-content-sha256");

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
        LOG_DEBUG() << req.socket.remote_endpoint()
                    << ": using chunked HMAC-SHA256";

        auto body = std::make_unique<chunk_body_sha256>(
            req, chunked_body::trailing_headers::none);

        return std::make_unique<http_request>(req, std::move(body));
    }

    if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        LOG_DEBUG() << req.socket.remote_endpoint()
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

std::unique_ptr<http_request> single_chunk_request(partial_parse_result& req) {

    auto content_length =
        std::stoul(req.optional("content-length").value_or("0"));

    if (!req.auth) {
        LOG_DEBUG() << req.socket.remote_endpoint()
                    << ": using single-chunk unauthenticated body";
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, content_length));
    }

    req.set_secret(SECRET_ACCESS_KEY);

    auto content_sha = req.require("x-amz-content-sha256");
    if (content_sha == "UNSIGNED-PAYLOAD") {
        LOG_DEBUG() << req.socket.remote_endpoint()
                    << ": using single-chunk unsigned body";
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, content_length));
    }

    // TODO insert check for payload signature
    LOG_DEBUG() << req.socket.remote_endpoint()
                << ": using single-chunk body with signed payload";
    return std::make_unique<http_request>(
        req, std::make_unique<raw_body>(req, content_length));
}

coro<std::unique_ptr<http_request>>
auth_request_factory::create(boost::asio::ip::tcp::socket& sock) {
    auto req = co_await partial_parse_result::read(sock);

    LOG_DEBUG() << sock.remote_endpoint()
                << ": pre-auth request: " << req.headers;

    auto transfer_encoding = req.optional("content-encoding");
    if (transfer_encoding && *transfer_encoding == "aws-chunked") {
        co_return multi_chunk_request(req);
    }

    co_return single_chunk_request(req);
}

} // namespace uh::cluster::ep::http
