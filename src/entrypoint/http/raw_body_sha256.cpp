#include "raw_body_sha256.h"

#include "common/utils/strings.h"

namespace uh::cluster::ep::http {

raw_body_sha256::raw_body_sha256(partial_parse_result& req, std::size_t length)
    : raw_body(req, length),
      m_signature(req.signature.value_or("")) {}

coro<std::size_t> raw_body_sha256::read(std::span<char> dest) {

    auto rv = co_await raw_body::read(dest);

    m_hash.consume(dest.subspan(0, rv));

    if (rv < dest.size()) {
        auto sig = to_hex(m_hash.finalize());
        if (sig != m_signature) {
            throw std::runtime_error("body signature mismatch");
        }
    }

    co_return rv;
}

} // namespace uh::cluster::ep::http
