#ifndef CORE_ENTRYPOINT_HTTP_AUTH_UTILS_H
#define CORE_ENTRYPOINT_HTTP_AUTH_UTILS_H

#include <map>
#include <set>
#include <string>

namespace uh::cluster::ep::http {

struct auth_info {
    /**
     * Construct auth_info by parsing Authorization header
     * Throws on parse error.
     */
    auth_info(std::string header);

    std::string signing_key(const std::string& secret) const;

    std::string header;

    std::string_view algorithm;
    std::string_view access_key_id;
    std::string_view date;
    std::string_view region;
    std::string_view service;
    std::set<std::string_view> signed_headers;
    std::string_view signature;
};

/**
 * Return the signing key for a given `auth_info` and a secret.
 */

std::map<std::string_view, std::string_view>
parse_values_string(std::string_view values, char pair_separator = ',',
                    char field_separator = '=');

} // namespace uh::cluster::ep::http

#endif
