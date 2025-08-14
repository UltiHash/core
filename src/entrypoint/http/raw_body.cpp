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

raw_body::raw_body(stream& s, raw_request& req)
    : m_s(s),
      m_length(get_length(req)) {}

std::optional<std::size_t> raw_body::length() const { return m_length; }

coro<std::span<const char>> raw_body::read(std::size_t count) {
    auto rv = co_await m_s.read(std::min(count, m_length));

    m_length -= rv.size();

    co_return rv;
}

coro<void> raw_body::consume() {
    co_await m_s.consume();
}

std::size_t raw_body::buffer_size() const {
    return m_s.buffer_size();
}

} // namespace uh::cluster::ep::http
