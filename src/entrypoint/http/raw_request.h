#pragma once

#include <common/types/common_types.h>

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

class raw_request {
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

    // Return part of the buffer after the headers (body data)
    std::span<const char> get_remained_buffer() const {
        return std::span<const char>(m_buffer).subspan(m_read_position);
    }

    // Return processed part of the buffer (header data)
    std::span<const char> get_raw_buffer() const {
        return std::span<const char>(m_buffer).subspan(0, m_read_position);
    }

    std::optional<std::string> optional(const std::string& name) const;

    std::string require(const std::string& name) const;

    boost::asio::ip::tcp::endpoint peer;
    std::map<std::string, std::string> params;
    std::string path;
    std::string encoded_path;

    beast::http::request<beast::http::empty_body> headers;

private:
    std::vector<char> m_buffer;
    std::size_t m_read_position = 0;

    // Shared header reading implementation - reads raw data only
    static coro<std::pair<std::vector<char>, size_t>>
    read_header_data(boost::asio::ip::tcp::socket& sock) {
        std::vector<char> buffer;

        auto header_length = co_await boost::asio::async_read_until(
            sock, boost::asio::dynamic_buffer(buffer), "\r\n\r\n",
            boost::asio::use_awaitable);

        co_return std::make_pair(std::move(buffer), header_length);
    }
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
