#include "auth_request_factory.h"

#include "auth_chunked_body.h"
#include "auth_utils.h"
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

std::string
require(const beast::http::request<beast::http::empty_body>& headers,
        const std::string& name) {

    auto header = headers.find(name);
    if (header == headers.end()) {
        throw std::runtime_error(name + " not found");
    }

    return header->value();
}

bool include_header(const std::string& name,
                    const std::set<std::string_view>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

std::string make_canonical_request(
    const beast::http::request<beast::http::empty_body>& headers,
    const auth_info& info) {

    auto url = parse_request_target(headers.target());

    // for details, see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    std::set<std::string> canonical_query_set;
    for (const auto& field : url.params) {
        canonical_query_set.emplace(url_encode(field.first) + "=" +
                                    url_encode(field.second));
    }

    std::string canonical_query = algorithm::join(canonical_query_set, "&");

    std::map<std::string, std::string> canonical_headers_map;
    for (const auto& header : headers) {
        auto name = lowercase(header.name_string());

        if (!include_header(name, info.signed_headers)) {
            continue;
        }

        canonical_headers_map[std::move(name)] =
            std::string(trim(header.value()));
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

    return std::string(headers.method_string()) + "\n" + url.path + "\n" +
           canonical_query + "\n" + canonical_headers + "\n" +
           signed_header_names + "\n" +
           require(headers, "x-amz-content-sha256");
}

} // namespace

constexpr const char* SECRET_ACCESS_KEY = "secret";

std::string operator+(std::string fst, std::string_view snd) {
    return fst + std::string(snd);
}

auth_info
read_auth_info(boost::asio::ip::tcp::socket& sock,
               const beast::http::request<beast::http::empty_body>& headers) {
    auto auth_header = headers.find("Authorization");
    if (auth_header == headers.end()) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": no Authorization header provided";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    try {
        return parse_auth_header(auth_header->value());
    } catch (const std::exception& e) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": error parsing authorization header: " << e.what();
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }
}

std::unique_ptr<http_request>
raw_chunk_auth_body(boost::asio::ip::tcp::socket& sock,
                    read_request_result& req) {

    auto info = read_auth_info(sock, req.headers);

    auto canonical_request = make_canonical_request(req.headers, info);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": canonical request: " << canonical_request;

    auto string_to_sign =
        std::string("AWS4-HMAC-SHA256\n") + require(req.headers, "x-amz-date") +
        "\n" + info.date + "/" + info.region + "/" + info.service +
        "/aws4_request\n" + sha256::from_string(canonical_request);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": string to sign: " << string_to_sign;

    // TODO request user information here and use user's secret key

    auto signing_key = make_signing_key(info, SECRET_ACCESS_KEY);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": signing key: " << to_hex(signing_key);

    auto signature =
        to_hex(hmac_sha256::from_string(signing_key, string_to_sign));
    if (signature != info.signature) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    std::size_t content_length = 0ull;
    if (auto cl = req.headers.find("Content-Length"); cl != req.headers.end()) {
        content_length = std::stoul(cl->value());
    }

    // TODO insert check for payload signature
    return std::make_unique<http_request>(
        std::move(req.headers),
        std::make_unique<raw_body>(sock, std::move(req.buffer), content_length),
        sock.remote_endpoint());
}

std::unique_ptr<http_request>
chunked_auth_body(boost::asio::ip::tcp::socket& sock,
                  read_request_result& req) {

    auto info = read_auth_info(sock, req.headers);
    if (!info.signed_headers.contains("content-encoding") ||
        !info.signed_headers.contains("content-length")) {
        throw std::runtime_error(
            "content headers are required for chunked transfer");
    }

    auto canonical_request = make_canonical_request(req.headers, info);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": canonical request: " << canonical_request;

    auto prelude = std::string("AWS4-HMAC-SHA256\n") +
                   require(req.headers, "x-amz-date") + "\n" + info.date + "/" +
                   info.region + "/" + info.service + "/aws4_request\n";

    auto string_to_sign = prelude + sha256::from_string(canonical_request);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": string to sign: " << string_to_sign;

    // TODO request user information here and use user's secret key

    prelude = std::string("AWS4-HMAC-SHA256-PAYLOAD\n") +
              require(req.headers, "x-amz-date") + "\n" + info.date + "/" +
              info.region + "/" + info.service + "/aws4_request\n";
    auto signing_key = make_signing_key(info, SECRET_ACCESS_KEY);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": signing key: " << to_hex(signing_key);

    auto signature =
        to_hex(hmac_sha256::from_string(signing_key, string_to_sign));
    if (signature != info.signature) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    return std::make_unique<http_request>(
        std::move(req.headers),
        std::make_unique<auth_chunked_body>(sock, std::move(req.buffer),
                                            prelude, signature, signing_key),
        sock.remote_endpoint());
}

coro<std::unique_ptr<http_request>>
auth_request_factory::create(boost::asio::ip::tcp::socket& sock) {
    auto req = co_await read_beast_request(sock);

    LOG_DEBUG() << sock.remote_endpoint()
                << ": pre-auth request: " << req.headers;

    auto content_sha = require(req.headers, "x-amz-content-sha256");
    if (content_sha == "UNSIGNED-PAYLOAD") {
        co_return raw_chunk_auth_body(sock, req);
    }

    if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        // TODO co_return std::move(request);
    }

    // TODO content-encoding: aws-chunked
    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD" ||
        content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER" ||
        content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD" ||
        content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD-TRAILER") {
        co_return chunked_auth_body(sock, req);
    }

    co_return raw_chunk_auth_body(sock, req);
}

} // namespace uh::cluster::ep::http
