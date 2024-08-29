#include "raw_body.h"

using namespace boost;

namespace uh::cluster::ep::http {

raw_body::raw_body(asio::ip::tcp::socket& s, beast::flat_buffer&& initial,
                   std::size_t length)
    : m_socket(s),
      m_buffer(std::move(initial)),
      m_length(length) {}

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
