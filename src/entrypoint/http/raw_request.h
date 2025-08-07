#pragma once

#include "common/types/common_types.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/url.hpp>

#include <map>
#include <optional>
#include <string>
#include <tuple>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;
using verb = beast::http::verb;
using status = beast::http::status;

struct raw_request {
    static coro<raw_request> read(boost::asio::ip::tcp::socket& sock);
    static raw_request
    from_string(beast::http::request<beast::http::empty_body> header,
                boost::asio::ip::tcp::endpoint peer, std::vector<char>&& buffer,
                size_t header_length);

    std::optional<std::string> optional(const std::string& name) const;
    std::string require(const std::string& name) const;

    beast::http::request<beast::http::empty_body> headers;
    boost::asio::ip::tcp::endpoint peer;

    std::map<std::string, std::string> params;
    std::string path;
    std::string encoded_path;

    auto get_remained_buffer() const {
        return std::span<const char>(buffer).subspan(read_position);
    }
    auto get_raw_buffer() const {
        return std::span<const char>(buffer).subspan(0, read_position);
    }

private:
    std::vector<char> buffer;
    std::size_t read_position = 0;
};

/**
 * Return bucket and object key.
 */
std::tuple<std::string, std::string>
extract_bucket_and_object(boost::urls::url url);

std::map<std::string_view, std::string_view>
parse_values_string(std::string_view values, char pair_separator = ',',
                    char field_separator = '=');

} // namespace uh::cluster::ep::http
