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

/*
 * Sync/source, which supports get/put API only
 */
#pragma once

#include <common/types/common_types.h>

namespace uh::cluster::proxy {

template <typename SocketType> class socket_sink {
public:
    socket_sink(SocketType& s)
        : m_s{s} {}

    coro<void> put(std::span<const char> sv) {
        if (sv.size() == 0) {
            co_return;
        }
        co_await boost::asio::async_write(m_s, boost::asio::buffer(sv));
    }

private:
    SocketType& m_s;
};

template <typename SocketType> class socket_source {
public:
    socket_source(SocketType& s)
        : m_s{s} {}

    coro<std::span<const char>> get(std::span<char> buffer) {
        auto n =
            co_await boost::asio::async_read(m_s, boost::asio::buffer(buffer));
        co_return std::span<const char>(buffer.data(), n);
    }

private:
    SocketType& m_s;
};

} // namespace uh::cluster::proxy
