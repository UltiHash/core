#include "chunked_body.h"

#include <charconv>

using namespace boost;

namespace uh::cluster::ep::http {

chunked_body::chunked_body(stream& s, trailing_headers trailing)
    : m_s(s),
      m_trailing(trailing) { }

std::optional<std::size_t> chunked_body::length() const { return {}; }

coro<std::span<const char>> chunked_body::read(std::size_t count) {
    if (m_end) {
        co_return std::span<const char>{};
    }

    if (m_chunk_bytes_left == 0ull) {
        auto hdr = co_await read_chunk_header();
        m_chunk_bytes_left = hdr.size;

        if (m_chunk_bytes_left == 0ull) {
            on_body_done();
            m_end = true;

            /*
            * Note: processing of trailing headers is not implemented yet.
            */
            if (m_trailing == trailing_headers::read) {
                co_await m_s.read_until("\r\n\r\n");
            } else {
                co_await m_s.read_until("\r\n");
            }

            co_return std::span<const char>{};
        }
    }

    auto n = std::min(m_chunk_bytes_left, count);
    auto data = co_await m_s.read(n);

    on_chunk_data(data);
    m_chunk_bytes_left -= data.size();

    if (m_chunk_bytes_left == 0ull) {
        on_chunk_done();

        auto nl = co_await m_s.read(2);
        if (nl[0] != '\r' || nl[1] != '\n') {
            throw std::runtime_error("newline required");
        }
    }

    co_return data;
}

coro<void> chunked_body::consume() {
    co_await m_s.consume();
}

std::size_t chunked_body::buffer_size() const {
    return m_s.buffer_size();
}

void chunked_body::on_chunk_header(const chunk_header&) {}
void chunked_body::on_chunk_data(std::span<const char>) {}
void chunked_body::on_chunk_done() {}
void chunked_body::on_body_done() {}

coro<chunked_body::chunk_header> chunked_body::read_chunk_header() {
    auto data = co_await m_s.read_until("\r\n");

    chunk_header hdr;

    auto [next, ec] = std::from_chars(data.data(), &data.back(), hdr.size, 16);
    if (ec != std::errc()) {
        throw std::runtime_error("from_chars failed: " +
                                 make_error_condition(ec).message());
    }

    if (*next == ';') {
        ++next;
        hdr.extensions_string = std::string(next, &data.back());
        hdr.extensions = parse_values_string(hdr.extensions_string, ';');
    }

    on_chunk_header(hdr);
    co_return hdr;
}

} // namespace uh::cluster::ep::http
