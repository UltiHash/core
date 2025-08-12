#include "raw_body.h"

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::size_t get_length(raw_request& req) {

    if (auto content_length = req.optional("content-length"); content_length) {
        return std::stoul(*content_length);
    }

    return 0ul;
}

} // namespace

raw_body::raw_body(boost::asio::ip::tcp::socket& sock, raw_request& req)
    : m_socket(sock),
      m_body_prefix(),
      m_length(get_length(req)) {

    m_body_prefix.resize(req.get_remained_buffer().size());
    asio::buffer_copy(asio::buffer(m_body_prefix),
                      asio::buffer(req.get_remained_buffer().data(),
                                   req.get_remained_buffer().size()));
}

raw_body::raw_body(boost::asio::ip::tcp::socket& sock, std::vector<char> b, std::size_t len)
     : m_socket(sock),
       m_body_prefix(std::move(b)),
       m_length(len) {}

std::optional<std::size_t> raw_body::length() const { return m_length; }

coro<std::size_t> raw_body::read(std::span<char> dest) {
    m_raw_buffers.clear();

    auto rv = 0ul;
    static_assert(std::is_same_v<decltype(rv), std::size_t>,
                  "auto rv = 0ul is not the same type as std::size_t");

    if (m_read_position < m_body_prefix.size()) {
        auto count =
            std::min(m_body_prefix.size() - m_read_position, dest.size());
        std::memcpy(&dest[0], m_body_prefix.data() + m_read_position, count);

        m_raw_buffers.push_back(
            {m_body_prefix.data() + m_read_position, count});
        m_read_position += count;
        rv += count;
        m_length -= count;
    }

    auto remaining_count = std::min(dest.size() - rv, m_length);
    if (remaining_count > 0) {
        auto read = co_await asio::async_read(
            m_socket, asio::buffer(&dest[rv], remaining_count),
            asio::use_awaitable);

        m_raw_buffers.push_back({&dest[rv], read});
        rv += read;
        m_length -= read;
    }

    co_return rv;
}

} // namespace uh::cluster::ep::http
