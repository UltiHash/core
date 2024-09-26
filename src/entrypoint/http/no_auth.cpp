#include "no_auth.h"

#include "chunked_body.h"
#include "raw_body.h"

namespace uh::cluster::ep::http {

coro<std::unique_ptr<request>> no_auth::create(partial_parse_result& req) {

    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        co_return std::make_unique<request>(
            std::move(req.headers), std::make_unique<chunked_body>(req),
            user::user{.name = user::user::ANONYMOUS}, std::move(req.peer));
    } else {
        co_return std::make_unique<request>(
            std::move(req.headers),
            std::make_unique<raw_body>(
                req, std::stoul(req.require("content-length"))),
            user::user{.name = user::user::ANONYMOUS}, std::move(req.peer));
    }
}

} // namespace uh::cluster::ep::http
