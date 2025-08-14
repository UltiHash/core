#include "string_body.h"

namespace uh::cluster::ep::http {

string_body::string_body(std::string&& body)
    : m_body(std::move(body)),
      m_read(0ull) {}

std::optional<std::size_t> string_body::length() const { return m_body.size(); }

coro<std::span<const char>> string_body::read(std::size_t count) {
    auto len = std::min(count, m_body.size() - m_read);

    auto rv = std::span{ &m_body[m_read], len};
    m_read += len;

    co_return rv;
}

coro<void> string_body::consume() {
    co_return;
}

std::size_t string_body::buffer_size() const {
    return m_body.size();
}

} // namespace uh::cluster::ep::http
