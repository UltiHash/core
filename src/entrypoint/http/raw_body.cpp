#include "raw_body.h"

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::size_t get_length(partial_parse_result& req) {

    if (auto content_length = req.optional("content-length"); content_length) {
        return std::stoul(*content_length);
    }

    return 0ull;
}

} // namespace

raw_body::raw_body(boost::asio::ip::tcp::socket& sock,
                   partial_parse_result& req)
    : m_socket(sock),
      m_buffer(std::move(req.buffer)),
      m_length(get_length(req)) {}

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
