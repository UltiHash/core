#include "raw_body_sha256.h"

#include <common/utils/strings.h>

namespace uh::cluster::ep::http {

raw_body_sha256::raw_body_sha256(stream& s,
                                 raw_request& req, std::string signature)
    : raw_body(s, req),
      m_signature(std::move(signature)) {}

coro<std::span<const char>> raw_body_sha256::read(std::size_t count) {

    auto rv = co_await raw_body::read(count);

    m_hash.consume(rv);

    if (rv.empty()) {
        auto sig = to_hex(m_hash.finalize());
        if (sig != m_signature) {
            throw std::runtime_error("body signature mismatch");
        }
    }

    co_return rv;
}

} // namespace uh::cluster::ep::http
