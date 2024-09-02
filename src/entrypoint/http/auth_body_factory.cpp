#include "auth_body_factory.h"

#include "chunk_body_sha256.h"
#include "raw_body.h"
#include "raw_body_sha256.h"

#include "common/telemetry/log.h"

namespace uh::cluster::ep::http {

auth_body_factory::auth_body_factory(std::unique_ptr<user::backend> user)
    : m_user(std::move(user)) {}

std::unique_ptr<body> auth_body_factory::create(partial_parse_result& req) {

    if (req.optional("content-encoding").value_or("") == "aws-chunked") {
        if (!req.auth) {
            LOG_INFO() << req.peer << ": unauthenticated chunked transfer";
            return std::make_unique<chunked_body>(req);
        }

        auto user = m_user->find(req.auth->access_key_id);
        req.set_secret(user.secret_key);

        auto content_sha = req.require("x-amz-content-sha256");

        if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
            LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256";
            return std::make_unique<chunk_body_sha256>(
                req, chunked_body::trailing_headers::none);
        }

        if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER") {
            LOG_DEBUG() << req.peer
                        << ": using chunked unsigned payload with trailer";
            return std::make_unique<chunked_body>(
                req, chunked_body::trailing_headers::read);
        }

        if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER") {
            LOG_DEBUG() << req.peer
                        << ": using chunked HMAC-SHA256 with trailer";
            return std::make_unique<chunk_body_sha256>(
                req, chunked_body::trailing_headers::read);
        }

        throw std::runtime_error("unsupported aws-chunked authentication: " +
                                 content_sha);
    } else {
        auto length = std::stoul(req.optional("content-length").value_or("0"));

        if (!req.auth) {
            LOG_DEBUG() << req.peer
                        << ": using single-chunk unauthenticated body";
            return std::make_unique<raw_body>(req, length);
        }

        auto user = m_user->find(req.auth->access_key_id);
        req.set_secret(user.secret_key);

        auto content_sha = req.require("x-amz-content-sha256");
        if (content_sha == "UNSIGNED-PAYLOAD") {
            LOG_DEBUG() << req.peer << ": using single-chunk unsigned body";
            return std::make_unique<raw_body>(req, length);
        }

        LOG_DEBUG() << req.peer
                    << ": using single-chunk body with signed payload";
        return std::make_unique<raw_body_sha256>(req, length);
    }
}

} // namespace uh::cluster::ep::http
