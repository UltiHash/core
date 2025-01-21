#pragma once

#include <entrypoint/http/request.h>
#include <entrypoint/user/db.h>

namespace uh::cluster::ep::http {

struct aws4_signature_info {
    std::string date;
    std::string region;
    std::string service;
};

class aws4_hmac_sha256 {
public:
    static coro<std::unique_ptr<request>>
    create(user::db& users, partial_parse_result& req, const std::string& auth);

    static coro<std::unique_ptr<request>>
    create_from_url(user::db& users, partial_parse_result& req);
};

} // namespace uh::cluster::ep::http
