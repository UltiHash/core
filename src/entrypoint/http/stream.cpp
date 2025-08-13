#include "stream.h"

namespace uh::cluster::ep::http {

socket_stream::socket_stream(boost::asio::ip::tcp::socket s,
                             std::size_t buffer_size)
    : m_s(std::move(s)),
      m_buffer_size(buffer_size),
      m_buffer(m_buffer_size),
      m_get_ptr(0ull),
      m_put_ptr(0ull) {}

coro<std::span<const char>> socket_stream::read(std::size_t count) {

    co_await fill();

    auto size = std::min(count, m_put_ptr - m_get_ptr);
    auto rv = std::span<const char>{ &m_buffer[m_get_ptr], size };

    m_get_ptr += size;

    co_return rv;
}

coro<std::span<const char>> socket_stream::read_until(std::string_view delimiter) {
    co_await fill();
    throw "not implemented";
}

void socket_stream::consume() {
    memmove(&m_buffer[0], &m_buffer[m_get_ptr], m_put_ptr - m_get_ptr);
    m_put_ptr -= m_get_ptr;
    m_get_ptr = 0;
}

coro<void> socket_stream::fill() {
    auto count = co_await m_s.async_read_some(
        boost::asio::buffer(&m_buffer[m_put_ptr], m_buffer.size() - m_put_ptr));

    m_put_ptr += count;
}

} // namespace uh::cluster::ep::http
