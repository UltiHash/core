#include "string_body.h"

namespace uh::cluster::ep::http {

string_body::string_body(std::string&& body)
    : m_body(std::move(body)) {}

std::optional<std::size_t> string_body::length() const { return m_body.size(); }

coro<std::size_t> string_body::read(std::span<char> buffer) {
    std::size_t count = std::min(buffer.size(), m_body.size());

    memcpy(buffer.data(), m_body.data(), count);
    m_body = m_body.substr(count);

    co_return count;
}

} // namespace uh::cluster::ep::http
