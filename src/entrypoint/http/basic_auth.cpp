#include "basic_auth.h"

#include "chunked_body.h"
#include "raw_body.h"
#include <common/utils/strings.h>

#include <entrypoint/http/request.h>

namespace uh::cluster::ep::http {

coro<std::unique_ptr<request>> basic_auth::create(user::db& users,
                                                  partial_parse_result& req) {

    auto header = req.require("authorization");
    std::size_t pos = header.find(' ');
    if (pos == std::string::npos) {
        throw std::runtime_error("no algorithm separator");
    }

    auto decoded = base64_decode({header.begin() + pos + 1, header.end()});
    auto creds = split({decoded.begin(), decoded.end()}, ':');

    if (creds.size() != 2) {
        throw std::runtime_error("credentials format error");
    }

    auto user = co_await users.find(creds[0], creds[1]);

    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        co_return std::make_unique<request>(
            std::move(req.headers), std::make_unique<chunked_body>(req),
            std::move(user), std::move(req.peer));
    } else {
        co_return std::make_unique<request>(
            std::move(req.headers),
            std::make_unique<raw_body>(
                req, std::stoul(req.require("content-length"))),
            std::move(user), std::move(req.peer));
    }
}

} // namespace uh::cluster::ep::http
