#include "auth_utils.h"

#include "common/crypto/hmac.h"
#include "common/utils/strings.h"

namespace uh::cluster::ep::http {

auth_info parse_auth_header(std::string header) {
    auth_info rv{.auth_header = std::move(header)};
    std::string_view h(rv.auth_header);

    std::size_t pos = h.find(' ');
    if (pos == std::string::npos) {
        throw std::runtime_error("no algorithm separator");
    }

    std::string_view algorithm(h.begin(), h.begin() + pos - 1);

    auto parsed = parse_values_string({h.begin() + pos + 1, h.end()});

    if (!parsed.contains("Credential") || !parsed.contains("SignedHeaders") ||
        !parsed.contains("Signature")) {
        throw std::runtime_error("required fields are missing");
    }

    auto split_credentials = split(parsed["Credential"], '/');
    if (split_credentials.size() != 5) {
        throw std::runtime_error("wrong size of crendentials");
    }

    rv.access_key_id = split_credentials[0];
    rv.date = split_credentials[1];
    rv.region = split_credentials[2];
    rv.service = split_credentials[3];
    rv.signed_headers =
        split<std::set<std::string_view>>(parsed["SignedHeaders"], ';');
    rv.signature = parsed["Signature"];

    return rv;
}

std::string make_signing_key(const auth_info& info, const std::string& secret) {
    auto date_key = hmac_sha256::from_string("AWS4" + secret, info.date);
    auto date_region_key = hmac_sha256::from_string(date_key, info.region);
    auto date_region_service_key =
        hmac_sha256::from_string(date_region_key, info.service);
    return hmac_sha256::from_string(date_region_service_key, "aws4_request");
}

std::map<std::string_view, std::string_view>
parse_values_string(std::string_view values, char pair_separator,
                    char field_separator) {

    auto pairs = split(values, pair_separator);

    std::map<std::string_view, std::string_view> rv;
    for (auto& pair : pairs) {
        auto parts = split(pair, '=');
        if (parts.size() != 2) {
            throw std::runtime_error(
                "more than two variables in values string");
        }

        rv[trim(parts[0])] = trim(parts[1]);
    }

    return rv;
}

} // namespace uh::cluster::ep::http
