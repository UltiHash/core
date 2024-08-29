#include "auth_request_factory.h"

#include "auth_utils.h"
#include "beast_utils.h"
#include "chunk_body_sha256.h"
#include "command_exception.h"
#include "common/crypto/hash.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "raw_body.h"

#include <boost/algorithm/string.hpp>
#include <set>

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

bool include_header(const std::string& name,
                    const std::set<std::string_view>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

std::string make_canonical_request(partial_parse_result& req) {

    auto url = parse_request_target(req.headers.target());

    // for details, see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    std::set<std::string> canonical_query_set;
    for (const auto& field : url.params) {
        canonical_query_set.emplace(url_encode(field.first) + "=" +
                                    url_encode(field.second));
    }

    std::string canonical_query = algorithm::join(canonical_query_set, "&");

    std::map<std::string, std::string> canonical_headers_map;
    for (const auto& header : req.headers) {
        auto name = lowercase(header.name_string());

        if (!include_header(name, req.auth->signed_headers)) {
            continue;
        }

        canonical_headers_map[std::move(name)] = trim(header.value());
    }

    std::string canonical_headers;
    std::string signed_header_names;
    bool first = true;

    for (const auto& header : canonical_headers_map) {
        if (!first) {
            signed_header_names += ";";
        }

        canonical_headers += header.first + ":" + header.second + "\n";
        signed_header_names += header.first;
        first = false;
    }

    return std::string(req.headers.method_string()) + "\n" + url.encoded_path +
           "\n" + canonical_query + "\n" + canonical_headers + "\n" +
           signed_header_names + "\n" + req.require("x-amz-content-sha256");
}

} // namespace

constexpr const char* SECRET_ACCESS_KEY = "secret";

std::string request_signature(partial_parse_result& req) {

    auto canonical_request = make_canonical_request(req);
    LOG_DEBUG() << req.socket.remote_endpoint()
                << ": canonical request: " << canonical_request;

    std::stringstream string_to_sign;
    string_to_sign << "AWS4-HMAC-SHA256\n"
                   << req.require("x-amz-date") << "\n"
                   << req.auth->date << "/" << req.auth->region << "/"
                   << req.auth->service << "/aws4_request\n"
                   << sha256::from_string(canonical_request);

    LOG_DEBUG() << req.socket.remote_endpoint()
                << ": string to sign: " << string_to_sign.str();

    return to_hex(
        hmac_sha256::from_string(*req.signing_key, string_to_sign.str()));
}

std::unique_ptr<http_request> multi_chunk_request(partial_parse_result& req) {
    auto content_length = std::stoul(req.require("content-length"));

    // TODO: can there be chunked transfer without auth?
    if (!req.auth) {
        return std::make_unique<http_request>(
            req, std::make_unique<raw_body>(req, content_length));
    }

    req.signing_key = req.auth->signing_key(SECRET_ACCESS_KEY);

    req.signature = request_signature(req);
    if (*req.signature != req.auth->signature) {
        LOG_DEBUG() << req.socket.remote_endpoint()
                    << ": access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

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

    req.signing_key = req.auth->signing_key(SECRET_ACCESS_KEY);
    req.signature = request_signature(req);
    if (*req.signature != req.auth->signature) {
        LOG_DEBUG() << req.socket.remote_endpoint()
                    << ": access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

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
