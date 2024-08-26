#include "auth_request_factory.h"

#include "auth_utils.h"
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

std::optional<std::string> optional(const auto& headers,
                                    const std::string& name) {

    if (auto header = headers.find(name); header != headers.end()) {
        return header->value();
    }

    return {};
}

std::string require(const auto& headers, const std::string& name) {

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

std::optional<auth_info>
read_auth_info(boost::asio::ip::tcp::socket& sock,
               const beast::http::request<beast::http::empty_body>& headers) {

    auto auth_header = optional(headers, "Authorization");
    if (!auth_header) {
        return {};
    }

    try {
        return parse_auth_header(*auth_header);
    } catch (const std::exception& e) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": error parsing authorization header: " << e.what();
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }
}

std::string request_signature(boost::asio::ip::tcp::socket& sock,
                              const read_request_result& req,
                              const auth_info& info, const std::string& key) {

    auto canonical_request = make_canonical_request(req.headers, info);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": canonical request: " << canonical_request;

    auto string_to_sign =
        std::string("AWS4-HMAC-SHA256\n") + require(req.headers, "x-amz-date") +
        "\n" + info.date + "/" + info.region + "/" + info.service +
        "/aws4_request\n" + sha256::from_string(canonical_request);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": string to sign: " << string_to_sign;

    return to_hex(hmac_sha256::from_string(key, string_to_sign));
}

std::unique_ptr<http_request>
multi_chunk_request(boost::asio::ip::tcp::socket& sock,
                    read_request_result req) {
    auto content_length = std::stoul(require(req.headers, "content-length"));

    // TODO: can there be chunked transfer without auth?
    auto info = read_auth_info(sock, req.headers);
    if (!info) {
        return std::make_unique<http_request>(
            std::move(req.headers),
            std::make_unique<raw_body>(sock, std::move(req.buffer),
                                       content_length),
            sock.remote_endpoint());
    }

    auto signing_key = make_signing_key(*info, SECRET_ACCESS_KEY);
    LOG_DEBUG() << sock.remote_endpoint()
                << ": signing key: " << to_hex(signing_key);

    auto signature = request_signature(sock, req, *info, signing_key);
    if (signature != info->signature) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auto content_sha = require(req.headers, "x-amz-content-sha256");

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
        LOG_DEBUG() << sock.remote_endpoint() << ": using chunked HMAC-SHA256";

        auto prelude = require(req.headers, "x-amz-date") + "\n" + info->date +
                       "/" + info->region + "/" + info->service +
                       "/aws4_request\n";

        return std::make_unique<http_request>(
            std::move(req.headers),
            std::make_unique<chunk_body_sha256>(
                sock, std::move(req.buffer),
                chunked_body::trailing_headers::none, "AWS4-HMAC-SHA256",
                prelude, signature, signing_key),
            sock.remote_endpoint());
    }

    if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": using chunked unsigned payload with trailer";

        return std::make_unique<http_request>(
            std::move(req.headers),
            std::make_unique<chunked_body>(
                sock, std::move(req.buffer),
                chunked_body::trailing_headers::read),
            sock.remote_endpoint());
    }

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER") {
    }

    throw std::runtime_error("unsupported aws-chunked authentication: " +
                             content_sha);
}

std::unique_ptr<http_request>
single_chunk_request(boost::asio::ip::tcp::socket& sock,
                     read_request_result req) {

    auto content_length =
        std::stoul(optional(req.headers, "content-length").value_or("0"));

    auto info = read_auth_info(sock, req.headers);
    if (!info) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": using single-chunk unauthenticated body";
        return std::make_unique<http_request>(
            std::move(req.headers),
            std::make_unique<raw_body>(sock, std::move(req.buffer),
                                       content_length),
            sock.remote_endpoint());
    }

    auto signing_key = make_signing_key(*info, SECRET_ACCESS_KEY);
    auto signature = request_signature(sock, req, *info, signing_key);
    if (signature != info->signature) {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auto content_sha = require(req.headers, "x-amz-content-sha256");
    if (content_sha == "UNSIGNED-PAYLOAD") {
        LOG_DEBUG() << sock.remote_endpoint()
                    << ": using single-chunk unsigned body";
        return std::make_unique<http_request>(
            std::move(req.headers),
            std::make_unique<raw_body>(sock, std::move(req.buffer),
                                       content_length),
            sock.remote_endpoint());
    }

    // TODO insert check for payload signature
    LOG_DEBUG() << sock.remote_endpoint()
                << ": using single-chunk body with signed payload";
    return std::make_unique<http_request>(
        std::move(req.headers),
        std::make_unique<raw_body>(sock, std::move(req.buffer), content_length),
        sock.remote_endpoint());
}

coro<std::unique_ptr<http_request>>
auth_request_factory::create(boost::asio::ip::tcp::socket& sock) {
    auto req = co_await read_beast_request(sock);

    LOG_DEBUG() << sock.remote_endpoint()
                << ": pre-auth request: " << req.headers;

    auto transfer_encoding = optional(req.headers, "content-encoding");
    if (transfer_encoding && *transfer_encoding == "aws-chunked") {
        co_return multi_chunk_request(sock, req);
    }

    co_return single_chunk_request(sock, std::move(req));
}

} // namespace uh::cluster::ep::http
