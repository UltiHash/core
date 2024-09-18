#ifndef CORE_ENTRYPOINT_HTTP_AUTH_UTILS_H
#define CORE_ENTRYPOINT_HTTP_AUTH_UTILS_H

#include "entrypoint/user/backend.h"
#include "entrypoint/user/user.h"
#include <map>
#include <optional>
#include <set>
#include <string>

namespace uh::cluster::ep::http {

struct partial_parse_result;

struct auth_info {
    /**
     * Construct auth_info by parsing Authorization header
     * Throws on parse error.
     */
    auth_info(std::string header);
    auth_info() = default;

    std::string header;

    // signing algorithm as given in Authorization header
    std::string_view algorithm;
    // access key id of signing user
    std::string_view access_key_id;
    // signing date as specified in header
    std::string_view date;
    // AWS region
    std::string_view region;
    // service ID ('s3')
    std::string_view service;
    // signed header as specified in request
    std::set<std::string_view> signed_headers;
    // signature of header as given in the header
    std::string_view header_signature;

    // computed signature of header
    std::optional<std::string> signature;
    // signing key of user
    std::optional<std::string> signing_key;
    // authenticated user information
    std::optional<user::user> authenticated_user;

    static coro<std::optional<auth_info>> create(partial_parse_result& req,
                                                 user::backend& users);
};

} // namespace uh::cluster::ep::http

#endif
