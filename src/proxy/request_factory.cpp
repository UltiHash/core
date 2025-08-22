#include "request_factory.h"

#include <entrypoint/http/chunked_body.h>
#include <entrypoint/http/raw_body.h>

namespace uh::cluster::proxy {

namespace {

using namespace uh::cluster::ep::http;

std::unique_ptr<body> make_body(stream& s, raw_request& req) {
    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        return std::make_unique<chunked_body>(s);
    }

    std::string content_sha =
        req.optional("x-amz-content-sha256").value_or("UNSIGNED-PAYLOAD");

    if (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {

        LOG_DEBUG() << req.peer << ": using chunked HMAC-SHA256";
        return std::make_unique<chunked_body>(
            s, chunked_body::trailing_headers::none);
    }

    if (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER" ||
        content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER") {

        LOG_DEBUG() << req.peer << ": using chunked reader with trailer";
        return std::make_unique<chunked_body>(s, chunked_body::trailing_headers::read);
    }

    LOG_DEBUG() << req.peer << ": using single-chunk body";
    return std::make_unique<raw_body>(s, req);
}

}

coro<std::unique_ptr<ep::http::request>> request_factory::create(ep::http::stream& s,
                                                       raw_request& req) {

    auto body = make_body(s, req);

    co_return std::make_unique<ep::http::request>(
            std::move(req), std::move(body),
            ep::user::user{.name = ep::user::user::ANONYMOUS});
}

} // namespace uh::cluster::proxy
