#include "raw_request.h"

#include "command_exception.h"

#include <boost/asio/buffer.hpp>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <sstream>

using namespace boost;

namespace uh::cluster::ep::http {

coro<raw_request> raw_request::read(stream& s,
                                    boost::asio::ip::tcp::endpoint peer) {
    auto buffer = co_await s.read_until("\r\n\r\n");

    beast::http::request_parser<beast::http::empty_body> parser;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    beast::error_code ec;
    parser.put(asio::buffer(buffer), ec);

    if (!parser.is_header_done()) {
        throw std::runtime_error("Incomplete HTTP header");
    }

    co_return from_string(parser.release(), peer);
}

raw_request
raw_request::from_string(beast::http::request<beast::http::empty_body> headers,
                         boost::asio::ip::tcp::endpoint peer) {
    raw_request rv;

    rv.headers = std::move(headers);
    if (rv.headers.version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    rv.peer = peer;

    const auto& target = rv.headers.target();
    auto query_index = target.find('?');

    boost::urls::url url;
    if (query_index != std::string::npos) {
        url.set_encoded_path(target.substr(0, query_index));
        url.set_encoded_query(target.substr(query_index + 1));
    } else {
        url.set_encoded_path(target);
    }

    rv.path = url.path();
    rv.encoded_path = url.encoded_path();

    for (const auto& param : url.params()) {
        rv.params[param.key] = param.value;
    }

    return rv;
}

std::optional<std::string>
raw_request::optional(const std::string& name) const {
    if (auto iter = headers.find(name); iter != headers.end()) {
        return iter->value();
    }
    return {};
}

std::string raw_request::require(const std::string& name) const {
    auto iter = headers.find(name);
    if (iter == headers.end()) {
        throw std::runtime_error(name + " not found");
    }
    return iter->value();
}

std::map<std::string_view, std::string_view>
parse_values_string(std::string_view values, char pair_separator,
                    char field_separator) {

    auto pairs = split(values, pair_separator);

    std::map<std::string_view, std::string_view> rv;
    for (auto& pair : pairs) {
        auto parts = split(pair, '=');
        if (parts.size() != 2) {
            throw std::runtime_error(
                "more than two variables in values string");
        }

        rv[trim(parts[0])] = trim(parts[1]);
    }

    return rv;
}

} // namespace uh::cluster::ep::http
