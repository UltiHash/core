#pragma once

#include <common/types/common_types.h>
#include <entrypoint/http/header.h>

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

class raw_request : public header<beast::http::request_header<>> {
public:
    static coro<raw_request> read(boost::asio::ip::tcp::socket& sock);
    static raw_request
    from_string(beast::http::request<beast::http::empty_body> header,
                boost::asio::ip::tcp::endpoint peer, std::vector<char>&& buffer,
                size_t header_length);
    raw_request() noexcept = default;
    raw_request(const raw_request&) = delete;
    raw_request& operator=(const raw_request&) = delete;
    raw_request(raw_request&&) noexcept = default;
    raw_request& operator=(raw_request&&) noexcept = default;

    boost::asio::ip::tcp::endpoint peer;
    std::map<std::string, std::string> params;
    std::string path;
    std::string encoded_path;
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
