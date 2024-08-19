#include "auth_request_factory.h"

#include "command_exception.h"
#include "common/crypto/hash.h"

#include <boost/algorithm/string.hpp>
#include <set>

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

struct auth_info {
    std::string_view algorithm;
    std::string_view access_key_id;
    std::string_view date;
    std::string_view region;
    std::string_view service;
    std::set<std::string_view> signed_headers;
    std::string_view signature;
};

auth_info parse_auth_header(std::optional<std::string> header) {
    if (!header) {
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    const auto& h = *header;
    std::size_t pos = h.find(' ');
    if (pos == std::string::npos) {
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }

    std::string_view algorithm(h.begin(), h.begin() + pos - 1);
    auto fields = split(std::string_view(h.begin() + pos + 1, h.end()), ',');

    std::map<std::string_view, std::string_view> parsed;

    for (auto& field : fields) {
        auto parts = split(field, '=');
        if (parts.size() != 2) {
            throw command_exception(
                beast::http::status::forbidden, "AuthorizationHeaderMalformed",
                "The authorization header that you provided is not valid.");
        }

        parsed[parts[0]] = parts[1];
    }

    if (!parsed.contains("Credential") || !parsed.contains("SignedHeaders") ||
        !parsed.contains("Signature")) {
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }

    auto split_crendentials = split(parsed["Credential"], '/');
    if (split_crendentials.size() != 5) {
        throw command_exception(
            beast::http::status::forbidden, "AuthorizationHeaderMalformed",
            "The authorization header that you provided is not valid.");
    }

    return auth_info{.algorithm = algorithm,
                     .access_key_id = split_crendentials[0],
                     .date = split_crendentials[1],
                     .region = split_crendentials[2],
                     .service = split_crendentials[3],
                     .signed_headers = split_set(parsed["SignedHeaders"], ';'),
                     .signature = parsed["Signature"]};
}

bool include_header(const std::string& name,
                    const std::set<std::string_view>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

coro<std::string> make_canonical_request(http_request& req,
                                         const auth_info& info) {
    // see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    // for details
    std::string method = beast::http::to_string(req.method());
    std::string canonical_uri =
        std::string(req.target()); // TODO still includes query?

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
        signed_headers.insert(name);
    }

    std::string canonical_headers =
        algorithm::join(canonical_header_set, "\n") + "\n";
    std::string signed_header_names = algorithm::join(signed_headers, ";");

    std::vector<char> payload;
    payload.reserve(req.content_length());
    auto size = co_await req.read_body(payload);
    payload.resize(size);

    std::string hashed_payload = sha256::from_buffer(payload);

    co_return method + "\n" + canonical_uri + "\n" + canonical_query + "\n" +
        canonical_headers + "\n" + signed_header_names + "\n" + hashed_payload;
}

} // namespace

auth_request_factory::auth_request_factory(
    std::unique_ptr<request_factory> base)
    : m_base(std::move(base)) {}

coro<std::unique_ptr<http_request>>
auth_request_factory::create(boost::asio::ip::tcp::socket& s) {
    auto request = co_await m_base->create(s);

    auto auth_header = request->header("Authorization");
    if (!auth_header) {
        LOG_DEBUG() << "no Authorization header provided";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auth_info info((std::string(*auth_header)));
    auto canonical_request = make_canonical_request(*request, info);

    LOG_DEBUG() << request->peer()
                << " canonical request: " << canonical_request;

    // TODO support for UNSIGNED-PAYLOAD & header based authentication

    auto string_to_sign =
        std::string("AWS4-HMAC-SHA256\n") +
        request->header("x-amz-date").value_or(std::string(info.date)) + "\n" +
        info.date + "/" + info.region + "/" + info.service + "/aws4_request\n" +
        sha256::from_string(canonical_request);

    LOG_DEBUG() << request->peer() << " string to sign: " << string_to_sign;

    // TODO request user information here and use user's secret key

    auto date_key =
        hmac_sha256::from_string("AWS4" + SECRET_ACCESS_KEY, info.date);
    auto date_region_key = hmac_sha256::from_string(date_key, info.region);
    auto date_region_service_key =
        hmac_sha256::from_string(date_region_key, info.service);
    auto signing_key =
        hmac_sha256::from_string(date_region_service_key, "aws4_request");

    LOG_DEBUG() << request->peer() << " signing key: " << to_hex(signing_key);

    auto signature =
        to_hex(hmac_sha256::from_string(signing_key, string_to_sign));
    if (signature != info.signature) {
        LOG_DEBUG() << request->peer() << " access denied: signature mismatch";
        throw command_exception(beast::http::status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    // TODO insert check for payload signature
    co_return std::move(request);
}

} // namespace uh::cluster::ep::http
