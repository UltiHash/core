#include "auth_request_factory.h"

#include "auth_utils.h"
#include "command_exception.h"
#include "common/crypto/hash.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"

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

std::string make_canonical_request(http_request& req, const auth_info& info) {
    // for details, see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    std::string method = beast::http::to_string(req.method());

    std::set<std::string> canonical_query_set;
    for (const auto& field : req.query_map()) {
        canonical_query_set.emplace(url_encode(field.first) + "=" +
                                    url_encode(field.second));
    }

    std::string canonical_query = algorithm::join(canonical_query_set, "&");

    std::set<std::string> canonical_header_set;
    std::set<std::string> signed_headers;
    for (const auto& header : req.header()) {
        auto name = lowercase(header.name_string());

        if (!include_header(name, info.signed_headers)) {
            continue;
        }

        canonical_header_set.insert(name + ":" +
                                    std::string(trim(header.value())));
        signed_headers.emplace(std::move(name));
    }

    auto canonical_headers = algorithm::join(canonical_header_set, "\n") + "\n";
    auto signed_header_names = algorithm::join(signed_headers, ";");

    return method + "\n" + req.path() + "\n" + canonical_query + "\n" +
           canonical_headers + "\n" + signed_header_names + "\n" +
           req.header("x-amz-content-sha256").value_or("");
}

} // namespace

auth_request_factory::auth_request_factory(
    std::unique_ptr<request_factory> base)
    : m_base(std::move(base)) {}

constexpr const char* SECRET_ACCESS_KEY = "secret";

std::string operator+(std::string fst, std::string_view snd) {
    return fst + std::string(snd);
}

std::unique_ptr<http_request>
raw_chunk_auth_body(boost::asio::ip::tcp::socket& sock,
                    std::unique_ptr<http_request>& req) {

    auto auth_header = req->header("Authorization");
    if (!auth_header) {
        LOG_DEBUG() << req->peer() << " no Authorization header provided";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auth_info info;
    try {
        info = parse_auth_header(std::move(*auth_header));
    } catch (const std::exception&) {
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }

    if (!info.signed_headers.contains("content-encoding") ||
        !info.signed_headers.contains("content-length")) {
        throw std::runtime_error(
            "content headers are required for chunked transfer");
    }

    auto canonical_request = make_canonical_request(*req, info);

    LOG_DEBUG() << req->peer() << " canonical request: " << canonical_request;

    auto string_to_sign =
        std::string("AWS4-HMAC-SHA256\n") +
        req->header("x-amz-date").value_or(std::string(info.date)) + "\n" +
        info.date + "/" + info.region + "/" + info.service + "/aws4_request\n" +
        sha256::from_string(canonical_request);

    LOG_DEBUG() << req->peer() << " string to sign: " << string_to_sign;

    // TODO request user information here and use user's secret key

    auto signing_key = make_signing_key(info, SECRET_ACCESS_KEY);
    LOG_DEBUG() << req->peer() << " signing key: " << to_hex(signing_key);

    auto signature =
        to_hex(hmac_sha256::from_string(signing_key, string_to_sign));
    if (signature != info.signature) {
        LOG_DEBUG() << req->peer() << " access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    // TODO insert check for payload signature
    return std::move(req);
}

std::unique_ptr<http_request>
chunked_auth_body(boost::asio::ip::tcp::socket& sock,
                  std::unique_ptr<http_request>& req) {

    auto auth_header = req->header("Authorization");
    if (!auth_header) {
        LOG_DEBUG() << req->peer() << " no Authorization header provided";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auth_info info;
    try {
        info = parse_auth_header(std::move(*auth_header));
    } catch (const std::exception&) {
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }

    auto canonical_request = make_canonical_request(*req, info);

    LOG_DEBUG() << req->peer() << " canonical request: " << canonical_request;

    auto string_to_sign =
        std::string("AWS4-HMAC-SHA256\n") +
        req->header("x-amz-date").value_or(std::string(info.date)) + "\n" +
        info.date + "/" + info.region + "/" + info.service + "/aws4_request\n" +
        sha256::from_string(canonical_request);

    LOG_DEBUG() << req->peer() << " string to sign: " << string_to_sign;

    // TODO request user information here and use user's secret key

    auto signing_key = make_signing_key(info, SECRET_ACCESS_KEY);

    LOG_DEBUG() << req->peer() << " signing key: " << to_hex(signing_key);

    auto signature =
        to_hex(hmac_sha256::from_string(signing_key, string_to_sign));
    if (signature != info.signature) {
        LOG_DEBUG() << req->peer() << " access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    return std::move(req);
}

coro<std::unique_ptr<http_request>>
auth_request_factory::create(boost::asio::ip::tcp::socket& sock) {
    auto request = co_await m_base->create(sock);

    LOG_DEBUG() << request->peer() << " pre-auth request: " << *request;

    auto content_sha = request->header("x-amz-content-sha256");
    if (!content_sha) {
        throw std::runtime_error(
            "X-AMZ-Content-SHA256 is required but was not provided");
    }

    if (*content_sha == "UNSIGNED-PAYLOAD") {
        co_return std::move(request);
    }

    if (*content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        co_return std::move(request);
    }

    if (*content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD" ||
        *content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER" ||
        *content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD" ||
        *content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD-TRAILER") {
        co_return chunked_auth_body(sock, request);
    }

    co_return raw_chunk_auth_body(sock, request);
}

} // namespace uh::cluster::ep::http
