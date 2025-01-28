#include "raw_body_sha256.h"

#include "common/utils/strings.h"

namespace uh::cluster::ep::http {

raw_body_sha256::raw_body_sha256(boost::asio::ip::tcp::socket& sock,
                                 raw_request& req, std::string signature)
    : raw_body(sock, req),
      m_signature(std::move(signature)) {}

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
