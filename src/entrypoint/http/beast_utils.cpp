#include "beast_utils.h"

using namespace boost;

namespace uh::cluster::ep::http {

coro<read_request_result> read_beast_request(asio::ip::tcp::socket& sock) {

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

    co_return read_request_result{req, buffer};
}

std::optional<std::string>
optional(const beast::http::request<beast::http::empty_body>& headers,
         const std::string& name) {

    if (auto header = headers.find(name); header != headers.end()) {
        return header->value();
    }

    return {};
}

std::string
require(const beast::http::request<beast::http::empty_body>& headers,
        const std::string& name) {

    auto header = headers.find(name);
    if (header == headers.end()) {
        throw std::runtime_error(name + " not found");
    }

    return header->value();
}

} // namespace uh::cluster::ep::http
