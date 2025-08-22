#include "no_auth.h"

#include "chunked_body.h"
#include "raw_body.h"

namespace uh::cluster::ep::http {

coro<std::unique_ptr<request>> no_auth::create(stream& s, raw_request req) {

    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        auto body = std::make_unique<chunked_body>(s);
        co_return std::make_unique<request>(
            std::move(req), std::move(body),
            user::user{.name = user::user::ANONYMOUS});
    } else {
        auto body = std::make_unique<raw_body>(s, req);
        co_return std::make_unique<request>(
            std::move(req), std::move(body),
            user::user{.name = user::user::ANONYMOUS});
    }
}

} // namespace uh::cluster::ep::http
