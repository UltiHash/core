#include "aws4_hmac_sha256.h"

#include "chunk_body_sha256.h"
#include "raw_body.h"
#include "raw_body_sha256.h"
#include "string_body.h"
#include <common/crypto/hmac.h>
#include <common/utils/strings.h>
#include <entrypoint/formats.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/http/request.h>

namespace uh::cluster::ep::http {

namespace {

std::set<std::string> QUERY_IGNORE_URL = {"X-Amz-Signature"};
std::set<std::string> QUERY_IGNORE_HEADER = {};

coro<std::unique_ptr<string_body>>
read_form_body(stream& s, raw_request& req) {
    auto content_type = req.optional("content-type");
    if (!content_type ||
        !content_type->starts_with("application/x-www-form-urlencoded")) {
        co_return std::unique_ptr<string_body>();
    }

    raw_body reader(s, req);
    std::size_t length = *reader.length();

    auto data = co_await reader.read(length);
    if (data.size() != length) {
        throw std::runtime_error("cannot read content-length bytes");
    }

    co_return std::make_unique<string_body>(std::string(data.data(), data.size()));
}

std::string make_signing_key(const std::string& secret,
                             const aws4_signature_info& info) {

    auto date_key = hmac_sha256::from_string("AWS4" + secret, info.date);
    auto date_region_key = hmac_sha256::from_string(date_key, info.region);
    auto date_region_service_key =
        hmac_sha256::from_string(date_region_key, info.service);

    return hmac_sha256::from_string(date_region_service_key, "aws4_request");
}

bool include_header(const std::string& name,
                    const std::set<std::string>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

std::string make_canonical_request(raw_request& req,
                                   const aws4_signature_info& info) {

    // for details, see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    std::set<std::string> canonical_query_set;
    for (const auto& field : req.params) {
        if (info.query_ignore.contains(field.first)) {
            continue;
        }

        canonical_query_set.emplace(uri_encode(field.first) + "=" +
                                    uri_encode(field.second));
    }

    std::string canonical_query = join(canonical_query_set, "&");

    std::map<std::string, std::string> canonical_headers_map;
    for (const auto& header : req.headers) {
        auto name = lowercase(header.name_string());

        if (!include_header(name, info.signed_headers)) {
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

    return std::string(req.headers.method_string()) + "\n" + req.encoded_path +
           "\n" + canonical_query + "\n" + canonical_headers + "\n" +
           signed_header_names + "\n" + info.content_sha;
}

std::string request_signature(raw_request& req, const aws4_signature_info& info,
                              const std::string& signing_key) {

    auto canonical_request = make_canonical_request(req, info);
    LOG_DEBUG() << req.peer << ": canonical request: " << canonical_request;

    std::stringstream string_to_sign;
    string_to_sign << "AWS4-HMAC-SHA256\n"
                   << info.amz_date << "\n"
                   << info.date << "/" << info.region << "/" << info.service
                   << "/aws4_request\n"
                   << to_hex(sha256::from_string(canonical_request));

    LOG_DEBUG() << req.peer << ": string to sign: " << string_to_sign.str();
    return to_hex(hmac_sha256::from_string(signing_key, string_to_sign.str()));
}

std::unique_ptr<body> make_body(stream& s,
                                raw_request& req,
                                const aws4_signature_info& info,
                                std::string signing_key,
                                std::string signature) {

    if (info.content_sha == "UNSIGNED-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using single-chunk unsigned body";
        return std::make_unique<raw_body>(s, req);
    }

    if (info.content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256";
        return std::make_unique<chunk_body_sha256>(
            s, req, info, signing_key, std::move(signature),
            chunked_body::trailing_headers::none);
    }

    if (info.content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
        LOG_DEBUG() << req.peer
                    << ": using chunked unsigned payload with trailer";
        return std::make_unique<chunked_body>(
            s, chunked_body::trailing_headers::read);
    }

    if (info.content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER") {
        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256 with trailer";
        return std::make_unique<chunk_body_sha256>(
            s, req, info, signing_key, std::move(signature),
            chunked_body::trailing_headers::read);
    }

    LOG_DEBUG() << req.peer << ": using single-chunk body with signed payload";
    return std::make_unique<raw_body_sha256>(s, req, info.content_sha);
}

} // namespace

coro<std::unique_ptr<request>>
aws4_hmac_sha256::create(stream& s, user::db& users,
                         raw_request req, const std::string& auth) {

    std::size_t pos = auth.find(' ');
    if (pos == std::string::npos) {
        throw std::runtime_error("no algorithm separator");
    }

    auto parsed = parse_values_string({auth.begin() + pos + 1, auth.end()});
    if (!parsed.contains("Credential") || !parsed.contains("SignedHeaders") ||
        !parsed.contains("Signature")) {
        throw std::runtime_error("required fields are missing");
    }

    auto split_credentials = split(parsed["Credential"], '/');
    if (split_credentials.size() != 5) {
        throw std::runtime_error("wrong size of crendentials");
    }

    auto user = co_await users.find_by_key(std::string(split_credentials[0]));

    std::string content_sha =
        req.optional("x-amz-content-sha256").value_or("UNSIGNED-PAYLOAD");

    std::unique_ptr<body> body;
    auto form_body = co_await read_form_body(s, req);
    if (form_body) {
        sha256 hash;
        const auto& s = form_body->get_body();
        hash.consume({s.data(), s.size()});
        content_sha = to_hex(hash.finalize());
        body = std::move(form_body);
    }

    aws4_signature_info info{.date = std::string(split_credentials[1]),
                             .region = std::string(split_credentials[2]),
                             .service = std::string(split_credentials[3]),
                             .signed_headers = split<std::set<std::string>>(
                                 parsed["SignedHeaders"], ';'),
                             .amz_date = req.require("x-amz-date"),
                             .content_sha = std::move(content_sha),
                             .query_ignore = QUERY_IGNORE_HEADER};

    auto signing_key = make_signing_key(user.access_key->secret_key, info);
    auto signature = request_signature(req, info, signing_key);

    if (signature != parsed["Signature"]) {
        LOG_INFO() << req.peer << ": access denied: signature mismatch";
        throw command_exception(status::forbidden, "InvalidAccessGrant",
                                "The specified Access Grant is invalid");
    }

    if (!body) {
        body = make_body(s, req, info, std::move(signing_key),
                         std::move(signature));
    }

    co_return std::make_unique<request>(std::move(req), std::move(body),
                                        std::move(user));
}

coro<std::unique_ptr<request>>
aws4_hmac_sha256::create_from_url(stream& s, user::db& users, raw_request req) {

    auto split_credentials = split(req.params["X-Amz-Credential"], '/');
    if (split_credentials.size() != 5) {
        throw std::runtime_error("wrong size of credentials");
    }

    aws4_signature_info info{.date = std::string(split_credentials[1]),
                             .region = std::string(split_credentials[2]),
                             .service = std::string(split_credentials[3]),
                             .signed_headers = split<std::set<std::string>>(
                                 req.params["X-Amz-SignedHeaders"], ';'),
                             .amz_date = req.params["X-Amz-Date"],
                             .content_sha = "UNSIGNED-PAYLOAD",
                             .query_ignore = QUERY_IGNORE_URL};

    auto date = read_iso8601_date_merged(info.amz_date);
    auto expires = std::chrono::seconds(stoul(req.params["X-Amz-Expires"]));
    if (date + expires < utc_time::clock::now()) {
        throw command_exception(status::bad_request, "ExpiredToken",
                                "The provided token has expired.");
    }

    auto user = co_await users.find_by_key(std::string(split_credentials[0]));
    auto signing_key = make_signing_key(user.access_key->secret_key, info);

    auto signature = request_signature(req, info, signing_key);

    if (signature != req.params["X-Amz-Signature"]) {
        LOG_INFO() << req.peer << ": access denied: signature mismatch";
        throw command_exception(
            status::forbidden, "SignatureDoesNotMatch",
            "SignatureDoesNotMatch	The request signature that the server "
            "calculated does not match the signature that you provided. Check "
            "your AWS secret access key and signing method. For more "
            "information, see REST Authentication and SOAP Authentication.");
    }

    req.params.erase("X-Amz-Algorithm");
    req.params.erase("X-Amz-Credential");
    req.params.erase("X-Amz-Date");
    req.params.erase("X-Amz-Expires");
    req.params.erase("X-Amz-Signature");
    req.params.erase("X-Amz-SignedHeaders");

    std::unique_ptr<body> body = co_await read_form_body(s, req);
    if (!body) {
        body = make_body(s, req, info, std::move(signing_key),
                         std::move(signature));
    }

    co_return std::make_unique<request>(std::move(req), std::move(body),
                                        std::move(user));
}

} // namespace uh::cluster::ep::http
