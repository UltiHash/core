#include "auth_utils.h"

#include "command_exception.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include "entrypoint/http/beast_utils.h"

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::string make_signing_key(const auth_info& auth, const std::string& secret) {

    auto date_key = hmac_sha256::from_string("AWS4" + secret, auth.date);
    auto date_region_key = hmac_sha256::from_string(date_key, auth.region);
    auto date_region_service_key =
        hmac_sha256::from_string(date_region_key, auth.service);

    return hmac_sha256::from_string(date_region_service_key, "aws4_request");
}

bool include_header(const std::string& name,
                    const std::set<std::string_view>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

std::string make_canonical_request(partial_parse_result& req,
                                   const auth_info& auth) {

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

        if (!include_header(name, auth.signed_headers)) {
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
                              const auth_info& auth) {

    auto canonical_request = make_canonical_request(req, auth);
    LOG_DEBUG() << req.peer << ": canonical request: " << canonical_request;

    std::stringstream string_to_sign;
    string_to_sign << "AWS4-HMAC-SHA256\n"
                   << req.require("x-amz-date") << "\n"
                   << auth.date << "/" << auth.region << "/" << auth.service
                   << "/aws4_request\n"
                   << to_hex(sha256::from_string(canonical_request));

    LOG_DEBUG() << req.peer << ": string to sign: " << string_to_sign.str();

    return to_hex(
        hmac_sha256::from_string(*auth.signing_key, string_to_sign.str()));
}

} // namespace

auth_info::auth_info(std::string hdr)
    : header(std::move(hdr)) {
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

    algorithm = std::string_view(header.begin(), header.begin() + pos - 1);
    access_key_id = split_credentials[0];
    date = split_credentials[1];
    region = split_credentials[2];
    service = split_credentials[3];
    signed_headers =
        split<std::set<std::string_view>>(parsed["SignedHeaders"], ';');
    header_signature = parsed["Signature"];
}

coro<std::optional<auth_info>> auth_info::create(partial_parse_result& req,
                                                 user::backend& users) {
    auto header = req.optional("authorization");
    if (!header) {
        co_return std::nullopt;
    }

    auth_info auth;
    try {
        auth = auth_info(*header);
    } catch (const std::exception& e) {
        LOG_INFO() << req.peer
                   << ": error parsing authorization header: " << e.what();
        throw command_exception(
            status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }

    auto user = co_await users.find(auth.access_key_id);
    auth.signing_key = make_signing_key(auth, user.secret_key);
    auth.signature = request_signature(req, auth);

    if (*auth.signature != auth.header_signature) {
        LOG_INFO() << req.peer << ": access denied: signature mismatch";
        throw command_exception(status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    co_return auth;
}

} // namespace uh::cluster::ep::http
