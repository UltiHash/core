#include "stream.h"

#include <format>

namespace uh::cluster::ep::http {

socket_stream::socket_stream(boost::asio::ip::tcp::socket& s,
                             std::size_t buffer_size)
    : m_s(s),
      m_buffer_size(buffer_size),
      m_buffer(m_buffer_size),
      m_get_ptr(0ull),
      m_put_ptr(0ull) {}

coro<std::span<const char>> socket_stream::read(std::size_t count) {

    if (count > m_put_ptr - m_get_ptr) {
        co_await fill();
    }

    auto size = std::min(count, m_put_ptr - m_get_ptr);
    auto rv = std::span<const char>{ &m_buffer[m_get_ptr], size };

    m_get_ptr += size;

    co_return rv;
}

coro<std::span<const char>> socket_stream::read_until(std::string_view delimiter) {
    std::size_t last_put = m_put_ptr;

    do {
        std::string_view current(&m_buffer[m_get_ptr], &m_buffer[m_put_ptr]);
        if (auto pos = current.find(delimiter); pos != std::string::npos) {
            auto rv = std::span<const char>{ &m_buffer[m_get_ptr], pos + delimiter.size() };
            m_get_ptr += pos + delimiter.size();
            co_return rv;
        }

        last_put = m_put_ptr;
        co_await fill();
    } while (m_put_ptr != last_put);

    co_return std::span<const char>{};
}

coro<std::size_t> socket_stream::write(std::span<const char> buffer) {
    co_return co_await async_write(m_s, buffer);
}

coro<void> socket_stream::consume() {
    memmove(&m_buffer[0], &m_buffer[m_get_ptr], m_put_ptr - m_get_ptr);
    m_put_ptr -= m_get_ptr;
    m_get_ptr = 0;
    co_return;
}

std::string socket_stream::peer() const {
    return std::format("session {}:{}",
        m_s.remote_endpoint().address().to_string(),
        m_s.remote_endpoint().port());
}

std::size_t socket_stream::buffer_size() const {
    return m_buffer_size;
}

std::span<const char> socket_stream::buffer() const {
    return { &m_buffer[0], m_get_ptr };
}

coro<void> socket_stream::fill() {
    auto count = co_await m_s.async_read_some(
        boost::asio::buffer(&m_buffer[m_put_ptr], m_buffer.size() - m_put_ptr));

    m_put_ptr += count;
}

} // namespace uh::cluster::ep::http
