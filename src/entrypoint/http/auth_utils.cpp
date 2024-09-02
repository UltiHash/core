#include "auth_utils.h"

#include "common/crypto/hmac.h"
#include "common/utils/strings.h"

namespace uh::cluster::ep::http {

auth_info::auth_info(std::string header)
    : header(std::move(header)) {
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
    signature = parsed["Signature"];
}

std::string auth_info::signing_key(const std::string& secret) const {

    auto date_key = hmac_sha256::from_string("AWS4" + secret, date);
    auto date_region_key = hmac_sha256::from_string(date_key, region);
    auto date_region_service_key =
        hmac_sha256::from_string(date_region_key, service);

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
