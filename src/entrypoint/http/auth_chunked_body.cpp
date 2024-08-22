#include "auth_chunked_body.h"

#include "auth_utils.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include <charconv>

using namespace boost;

namespace uh::cluster::ep::http {

auth_chunked_body::auth_chunked_body(asio::ip::tcp::socket& s,
                                     const beast::flat_buffer& initial,
                                     const std::string& prelude,
                                     const std::string& seed,
                                     const std::string& signing_key)
    : m_socket(s),
      m_buffer(),
      m_signature_prelude(prelude),
      m_signing_key(signing_key),
      m_string_to_sign(m_signature_prelude + seed + "\n" + SHA256_EMPTY_STRING +
                       "\n") {
    m_buffer.reserve(BUFFER_SIZE);
    m_buffer.resize(initial.size());
    asio::buffer_copy(asio::buffer(m_buffer),
                      asio::buffer(initial.data(), initial.size()));

    LOG_DEBUG() << "seed: " << seed;
}

coro<std::size_t> auth_chunked_body::read(std::span<char> dest) {
    if (m_end) {
        throw std::runtime_error("trying to read past end of data");
    }

    std::size_t rv = 0ull;

    while (rv < dest.size()) {
        if (m_chunk_size == 0ull) {
            m_chunk_size = co_await read_chunk_header();
        }

        if (m_chunk_size == 0ull) {
            m_end = true;

            m_string_to_sign += SHA256_EMPTY_STRING;
            LOG_DEBUG() << "final chunked string to sign: " << m_string_to_sign;
            auto signature = to_hex(
                hmac_sha256::from_string(m_signing_key, m_string_to_sign));
            if (signature != m_chunk_signature) {
                throw std::runtime_error("chunk signature mismatch: '" +
                                         signature + "' != '" +
                                         m_chunk_signature + "'");
            }

            break;
        }

        auto count = std::min(m_chunk_size, dest.size() - rv);
        auto read = co_await read_data(dest.subspan(rv, count));
        m_hash.consume(dest.subspan(rv, read));

        rv += read;
        m_chunk_size -= read;

        if (m_chunk_size == 0ull) {
            m_string_to_sign += m_hash.finalize();
            m_hash.reset();
            LOG_DEBUG() << "chunked string to sign: " << m_string_to_sign;

            auto signature = to_hex(
                hmac_sha256::from_string(m_signing_key, m_string_to_sign));
            if (signature != m_chunk_signature) {
                throw std::runtime_error("chunk signature mismatch: '" +
                                         signature + "' != '" +
                                         m_chunk_signature + "'");
            }

            m_string_to_sign = m_signature_prelude + signature + "\n" +
                               SHA256_EMPTY_STRING + "\n";
        }

        co_await read_nl();
    }

    co_return rv;
}

coro<void> auth_chunked_body::read_nl() {
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

std::size_t auth_chunked_body::find_nl() const {
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

coro<std::size_t> auth_chunked_body::read_chunk_header() {
    co_await asio::async_read_until(m_socket, asio::dynamic_buffer(m_buffer),
                                    "\r\n", asio::use_awaitable);

    auto pos = find_nl();

    std::size_t size = 0ull;
    auto [next, ec] = std::from_chars(&m_buffer[0], &m_buffer[pos], size, 16);

    if (ec != std::errc()) {
        throw std::runtime_error("from_chars failed: " +
                                 make_error_condition(ec).message());
    }

    // next points to ';'
    if (next == &m_buffer[pos] || *next != ';') {
        throw std::runtime_error("chunk header flags missing");
    }

    ++next;
    // TODO return all parts of chunk header (or at least signature)
    std::string extensions(next, &*(m_buffer.cbegin() + pos));
    auto parsed = parse_values_string(extensions, ';');
    if (!parsed.contains("chunk-signature")) {
        throw std::runtime_error("chunk signature is missing");
    }

    m_chunk_signature = parsed["chunk-signature"];

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + pos);
    co_return size;
}

coro<std::size_t> auth_chunked_body::read_data(std::span<char> buffer) {
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
