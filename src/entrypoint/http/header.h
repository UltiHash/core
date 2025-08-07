#pragma once

#include "common/types/common_types.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;

// Base class for HTTP headers (request or response)
template <typename header_t> class header {
public:
    std::optional<std::string> optional(const std::string& name) const {
        if (auto iter = headers.find(name); iter != headers.end()) {
            return iter->value();
        }
        return {};
    }

    std::string require(const std::string& name) const {
        auto iter = headers.find(name);
        if (iter == headers.end()) {
            throw std::runtime_error(name + " not found");
        }
        return iter->value();
    }

    header_t headers;

    // Return part of the buffer after the headers (body data)
    std::span<const char> get_remained_buffer() const {
        return std::span<const char>(m_buffer).subspan(m_read_position);
    }

    // Return processed part of the buffer (header data)
    std::span<const char> get_raw_buffer() const noexcept {
        return std::span<const char>(m_buffer).subspan(0, m_read_position);
    }

protected:
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

} // namespace uh::cluster::ep::http
