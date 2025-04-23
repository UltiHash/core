#pragma once

#include <entrypoint/http/request.h>
#include <entrypoint/user/db.h>
#include <set>

namespace uh::cluster::ep::http {

struct aws4_signature_info {
    std::string date;
    std::string region;
    std::string service;
    std::set<std::string> signed_headers;
    std::string amz_date;
    std::string content_sha;
    std::set<std::string>& query_ignore;
};

class aws4_hmac_sha256 {
public:
    static coro<std::unique_ptr<request>>
    create(boost::asio::ip::tcp::socket& s, user::db& users, raw_request req,
           const std::string& auth);

    static coro<std::unique_ptr<request>>
    create_from_url(boost::asio::ip::tcp::socket& s, user::db& users,
                    raw_request req);
};

} // namespace uh::cluster::ep::http
