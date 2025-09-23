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

namespace std {

template <typename T>
inline std::span<const char> get_span(const std::vector<T>& v) {
    return {reinterpret_cast<const char*>(v.data()), v.size() * sizeof(T)};
}

template <typename T> inline std::span<char> get_span(std::vector<T>& v) {
    return {reinterpret_cast<char*>(v.data()), v.size() * sizeof(T)};
}

inline std::span<const char> get_span(const std::string& s) {
    return {s.data(), s.size()};
}

inline std::span<char> get_span(std::string& s) { return {s.data(), s.size()}; }

} // namespace std
