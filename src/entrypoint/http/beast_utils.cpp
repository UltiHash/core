#include "beast_utils.h"

#include "command_exception.h"
#include "common/utils/strings.h"

using namespace boost;

namespace uh::cluster::ep::http {

coro<partial_parse_result>
partial_parse_result::read(asio::ip::tcp::socket& sock) {

    beast::http::request_parser<beast::http::empty_body> parser;
    beast::flat_buffer buffer;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(sock, buffer, parser,
                                            asio::use_awaitable);

    co_return from_string(std::move(parser.get()), std::move(buffer),
                          sock.remote_endpoint());
}

partial_parse_result partial_parse_result::from_string(
    beast::http::request<beast::http::empty_body> headers,
    beast::flat_buffer buffer, boost::asio::ip::tcp::endpoint peer) {

    partial_parse_result rv;

    rv.headers = std::move(headers);
    if (rv.headers.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    rv.buffer = std::move(buffer);
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

    // TODO this is S3 responsibility and should not be handled here
    auto keys = extract_bucket_and_object(url);

    rv.bucket = std::move(std::get<0>(keys));
    rv.object = std::move(std::get<1>(keys));

    return rv;
}

std::optional<std::string>
partial_parse_result::optional(const std::string& name) const {

    if (auto header = headers.find(name); header != headers.end()) {
        return header->value();
    }

    return {};
}

std::string partial_parse_result::require(const std::string& name) const {

    auto header = headers.find(name);
    if (header == headers.end()) {
        throw std::runtime_error(name + " not found");
    }

    return header->value();
}

std::tuple<std::string, std::string>
extract_bucket_and_object(boost::urls::url url) {
    std::string bucket_id;
    std::string object_key;

    for (const auto& seg : url.segments()) {
        if (bucket_id.empty())
            bucket_id = seg;
        else
            object_key += seg + '/';
    }

    if (!object_key.empty())
        object_key.pop_back();

    return std::make_tuple(bucket_id, object_key);
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
