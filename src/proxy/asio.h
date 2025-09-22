#pragma once

#include <boost/asio/buffer.hpp>
#include <span>

namespace boost::asio {
inline std::span<const char> get_span(boost::asio::const_buffer buffer) {
    return {static_cast<const char*>(buffer.data()), buffer.size()};
}

inline std::span<char> get_span(boost::asio::mutable_buffer buffer) {
    return {static_cast<char*>(buffer.data()), buffer.size()};
}
} // namespace boost::asio
