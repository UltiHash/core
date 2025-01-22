#include "basic_auth.h"

#include "chunked_body.h"
#include "raw_body.h"
#include <common/utils/strings.h>

namespace uh::cluster::ep::http {

coro<std::unique_ptr<request>>
basic_auth::create(boost::asio::ip::tcp::socket& s, user::db& users,
                   partial_parse_result req) {

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

    auto user = co_await users.find_and_check(std::string(creds[0]),
                                              std::string(creds[1]));

    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        auto body = std::make_unique<chunked_body>(s, req);
        co_return std::make_unique<request>(std::move(req), std::move(body),
                                            std::move(user));
    } else {
        std::size_t length =
            std::stoul(req.optional("content-length").value_or("0"));
        auto body = std::make_unique<raw_body>(s, req, length);
        co_return std::make_unique<request>(std::move(req), std::move(body),
                                            std::move(user));
    }
}

} // namespace uh::cluster::ep::http
