#include "raw_body.h"

using namespace boost;

namespace uh::cluster::ep::http {

raw_body::raw_body(partial_parse_result& req, std::size_t length)
    : m_socket(req.socket),
      m_buffer(std::move(req.buffer)),
      m_length(length) {}

std::optional<std::size_t> raw_body::length() const { return m_length; }

coro<std::size_t> raw_body::read(std::span<char> dest) {
    auto rv = 0ull;

    if (m_buffer.size() > 0ull) {
        auto count = asio::buffer_copy(asio::buffer(&dest[0], dest.size()),
                                       m_buffer.data());
        m_buffer.consume(count);
        rv += count;
        m_length -= count;
    }

    auto count = std::min(dest.size(), m_length);
    auto read = co_await asio::async_read(
        m_socket, asio::buffer(&dest[rv], count), asio::use_awaitable);

    rv += read;
    m_length -= read;
    co_return rv;
}

} // namespace uh::cluster::ep::http
