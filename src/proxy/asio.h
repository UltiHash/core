/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
