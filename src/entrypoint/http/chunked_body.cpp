#include "chunked_body.h"

#include "auth_utils.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"

#include <charconv>

using namespace boost;

namespace uh::cluster::ep::http {

chunked_body::chunked_body(partial_parse_result& req, trailing_headers trailing)
    : m_socket(req.socket),
      m_buffer(),
      m_trailing(trailing) {
    m_buffer.reserve(BUFFER_SIZE);
    m_buffer.resize(req.buffer.size());
    asio::buffer_copy(asio::buffer(m_buffer),
                      asio::buffer(req.buffer.data(), req.buffer.size()));
}

coro<std::size_t> chunked_body::read(std::span<char> dest) {
    if (m_end) {
        throw std::runtime_error("trying to read past end of data");
    }

    std::size_t rv = 0ull;

    while (rv < dest.size()) {
        if (m_chunk_size == 0ull) {
            auto hdr = co_await read_chunk_header();
            m_chunk_size = hdr.size;

            on_chunk_header(hdr);
        }

        if (m_chunk_size == 0ull) {
            on_body_done();
            m_end = true;

            if (m_trailing == trailing_headers::read) {
                LOG_DEBUG()
                    << m_socket.remote_endpoint()
                    << " reading trailing headers, bytes in buffer: "
                    << m_buffer.size()
                    << to_hex(std::string(&m_buffer[0], m_buffer.size()));

                co_await asio::async_read_until(
                    m_socket, asio::dynamic_buffer(m_buffer), "\r\n\r\n",
                    asio::use_awaitable);

                /*
                 * Note: here we should parse the headers and pass the result
                 * to some callback `on_trailing_headers`.
                 * However, there is no direct support from beast to partially
                 * parse HTTP headers without a full request object and we do
                 * not consider this functionality required at the moment, so
                 * we leave only this note here.
                 */
            }

            break;
        }

        auto count = std::min(m_chunk_size, dest.size() - rv);
        auto read = co_await read_data(dest.subspan(rv, count));

        on_chunk_data(dest.subspan(rv, read));

        rv += read;
        m_chunk_size -= read;

        if (m_chunk_size == 0ull) {
            on_chunk_done();
        }

        co_await read_nl();
    }

    co_return rv;
}

void chunked_body::on_chunk_header(const chunk_header&) {}
void chunked_body::on_chunk_data(std::span<char>) {}
void chunked_body::on_chunk_done() {}
void chunked_body::on_body_done() {}

coro<void> chunked_body::read_nl() {
    char nl[2];
    std::size_t offset = 0;

    if (!m_buffer.empty()) {
        auto count = std::min(m_buffer.size(), sizeof(nl) - offset);
        memcpy(nl, &m_buffer[0], count);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + count);
        offset += count;
    }

    if (offset < sizeof(nl)) {
        co_await asio::async_read(
            m_socket, asio::buffer(&nl[offset], sizeof(nl) - offset),
            asio::use_awaitable);
    }

    if (nl[0] != '\r' || nl[1] != '\n') {
        throw std::runtime_error("newline required");
    }
}

std::size_t chunked_body::find_nl() const {
    bool has_cr = false;

    for (auto index = 0ull; index < m_buffer.size(); ++index) {
        if (m_buffer[index] == '\r') {
            has_cr = true;
            continue;
        }

        if (m_buffer[index] == '\n' && has_cr) {
            return index + 1;
        }

        has_cr = false;
    }

    throw std::runtime_error("newline required");
}

coro<chunked_body::chunk_header> chunked_body::read_chunk_header() {
    co_await asio::async_read_until(m_socket, asio::dynamic_buffer(m_buffer),
                                    "\r\n", asio::use_awaitable);

    auto pos = find_nl();

    chunk_header hdr;

    auto [next, ec] =
        std::from_chars(&m_buffer[0], &m_buffer[pos], hdr.size, 16);
    if (ec != std::errc()) {
        throw std::runtime_error("from_chars failed: " +
                                 make_error_condition(ec).message());
    }

    if (next != &m_buffer[pos] && *next == ';') {
        ++next;
        hdr.extensions_string = std::string(next, &*(m_buffer.cbegin() + pos));
        hdr.extensions = parse_values_string(hdr.extensions_string, ';');
    }

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + pos);
    co_return hdr;
}

coro<std::size_t> chunked_body::read_data(std::span<char> buffer) {
    std::size_t offs = 0ull;

    if (m_buffer.size() > 0) {
        auto count = std::min(m_buffer.size(), buffer.size());
        memcpy(buffer.data(), m_buffer.data(), count);

        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + count);
        offs += count;
    }

    if (offs < buffer.size()) {
        offs += co_await asio::async_read(
            m_socket, asio::buffer(buffer.data() + offs, buffer.size() - offs),
            asio::use_awaitable);
    }

    co_return offs;
}

} // namespace uh::cluster::ep::http
