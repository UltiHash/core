#include "beast_utils.h"

#include "command_exception.h"
#include "common/telemetry/log.h"

using namespace boost;

namespace uh::cluster::ep::http {

coro<partial_parse_result>
partial_parse_result::read(asio::ip::tcp::socket& sock) {

    beast::http::request_parser<beast::http::empty_body> parser;
    beast::flat_buffer buffer;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(sock, buffer, parser,
                                            asio::use_awaitable);

    auto req = std::move(parser.get());
    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    auto rv = partial_parse_result{sock, std::move(buffer), std::move(req)};

    if (auto authorization = rv.optional("authorization"); authorization) {
        try {
            rv.auth = auth_info(*authorization);
        } catch (const std::exception& e) {
            LOG_DEBUG() << sock.remote_endpoint()
                        << ": error parsing authorization header: " << e.what();
            throw command_exception(
                beast::http::status::forbidden, "AuthorizationHeaderMalformed",
                "The authorization header that you provided is not valid.");
        }
    }

    co_return rv;
}

std::optional<std::string>
partial_parse_result::optional(const std::string& name) {

    if (auto header = headers.find(name); header != headers.end()) {
        return header->value();
    }

    return {};
}

std::string partial_parse_result::require(const std::string& name) {

    auto header = headers.find(name);
    if (header == headers.end()) {
        throw std::runtime_error(name + " not found");
    }

    return header->value();
}

} // namespace uh::cluster::ep::http
