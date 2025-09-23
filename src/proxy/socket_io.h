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
