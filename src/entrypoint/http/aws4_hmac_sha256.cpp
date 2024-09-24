#include "aws4_hmac_sha256.h"

#include "chunk_body_sha256.h"
#include "raw_body.h"
#include "raw_body_sha256.h"
#include <common/crypto/hmac.h>
#include <common/utils/strings.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/http/request.h>

namespace uh::cluster::ep::http {

namespace {

std::string make_signing_key(std::string_view secret,
                             const aws4_signature_info& info) {

    auto date_key =
        hmac_sha256::from_string("AWS4" + std::string(secret), info.date);
    auto date_region_key = hmac_sha256::from_string(date_key, info.region);
    auto date_region_service_key =
        hmac_sha256::from_string(date_region_key, info.service);

    return hmac_sha256::from_string(date_region_service_key, "aws4_request");
}

bool include_header(const std::string& name,
                    const std::set<std::string_view>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

std::string
make_canonical_request(partial_parse_result& req,
                       const std::set<std::string_view>& signed_headers) {

    auto url = parse_request_target(req.headers.target());

    // for details, see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    std::set<std::string> canonical_query_set;
    for (const auto& field : url.params) {
        canonical_query_set.emplace(uri_encode(field.first) + "=" +
                                    uri_encode(field.second));
    }

    std::string canonical_query = join(canonical_query_set, "&");

    std::map<std::string, std::string> canonical_headers_map;
    for (const auto& header : req.headers) {
        auto name = lowercase(header.name_string());

        if (!include_header(name, signed_headers)) {
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

std::string request_signature(partial_parse_result& req,
                              const aws4_signature_info& info,
                              const std::set<std::string_view>& signed_headers,
                              const std::string& signing_key) {

    auto canonical_request = make_canonical_request(req, signed_headers);
    LOG_DEBUG() << req.peer << ": canonical request: " << canonical_request;

    std::stringstream string_to_sign;
    string_to_sign << "AWS4-HMAC-SHA256\n"
                   << req.require("x-amz-date") << "\n"
                   << info.date << "/" << info.region << "/" << info.service
                   << "/aws4_request\n"
                   << to_hex(sha256::from_string(canonical_request));

    LOG_DEBUG() << req.peer << ": string to sign: " << string_to_sign.str();
    return to_hex(hmac_sha256::from_string(signing_key, string_to_sign.str()));
}

std::unique_ptr<body> make_body(partial_parse_result& req,
                                const aws4_signature_info& info,
                                std::string signing_key,
                                std::string signature) {
    auto length = std::stoul(req.optional("content-length").value_or("0"));

    auto content_sha = req.require("x-amz-content-sha256");

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256";
        return std::make_unique<chunk_body_sha256>(
            req, info, signing_key, signature,
            chunked_body::trailing_headers::none);
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
            req, info, signing_key, signature,
            chunked_body::trailing_headers::read);
    }

    if (content_sha == "UNSIGNED-PAYLOAD") {
        LOG_DEBUG() << req.peer << ": using single-chunk unsigned body";
        return std::make_unique<raw_body>(req, length);
    }

    LOG_DEBUG() << req.peer << ": using single-chunk body with signed payload";
    return std::make_unique<raw_body_sha256>(req, signature, length);
}

} // namespace

coro<std::unique_ptr<request>>
aws4_hmac_sha256::create(user::db& users, partial_parse_result& req) {

    auto header = req.require("authorization");
    std::size_t pos = header.find(' ');
    if (pos == std::string::npos) {
        throw std::runtime_error("no algorithm separator");
    }

    auto parsed = parse_values_string({header.begin() + pos + 1, header.end()});
    if (!parsed.contains("Credential") || !parsed.contains("SignedHeaders") ||
        !parsed.contains("Signature")) {
        throw std::runtime_error("required fields are missing");
    }

    auto split_credentials = split(parsed["Credential"], '/');
    if (split_credentials.size() != 5) {
        throw std::runtime_error("wrong size of crendentials");
    }

    aws4_signature_info info{
        .date = std::string(split_credentials[1]),
        .region = std::string(split_credentials[2]),
        .service = std::string(split_credentials[3]),
    };

    auto signed_headers =
        split<std::set<std::string_view>>(parsed["SignedHeaders"], ';');
    auto user = co_await users.find(split_credentials[0]);
    auto signing_key = make_signing_key(user.secret_key, info);
    auto signature = request_signature(req, info, signed_headers, signing_key);

    if (signature != parsed["Signature"]) {
        LOG_INFO() << req.peer << ": access denied: signature mismatch";
        throw command_exception(status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auto body = make_body(req, std::move(info), std::move(signature),
                          std::move(signing_key));
    co_return std::make_unique<request>(std::move(req.headers), std::move(body),
                                        std::move(user), std::move(req.peer));
}

} // namespace uh::cluster::ep::http
