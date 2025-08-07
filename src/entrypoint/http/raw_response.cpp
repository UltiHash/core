#include "raw_response.h"

#include <boost/asio/buffer.hpp>
#include <common/telemetry/log.h>

using namespace boost;

namespace uh::cluster::ep::http {

coro<raw_response> raw_response::read(asio::ip::tcp::socket& sock) {
    auto [buffer, header_length] = co_await header::read_header_data(sock);

    // Parse the header
    beast::http::response_parser<beast::http::empty_body> parser;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    beast::error_code ec;
    parser.put(asio::buffer(buffer), ec);

    if (!parser.is_header_done()) {
        throw std::runtime_error("Incomplete HTTP header");
    }

    co_return from_string(parser.release(), std::move(buffer), header_length);
}

raw_response raw_response::from_string(
    beast::http::response<beast::http::empty_body> headers,
    std::vector<char>&& buffer, size_t header_length) {
    raw_response rv;

    rv.headers = std::move(headers);
    if (rv.headers.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    rv.m_buffer = std::move(buffer);
    rv.m_read_position = header_length;

    return rv;
}

} // namespace uh::cluster::ep::http
