#include "chunk_body_sha256.h"

#include "auth_utils.h"
#include "beast_utils.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include <charconv>

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::string make_prelude(const auto& headers, const auto& info) {
    return "AWS4-HMAC-SHA256-PAYLOAD\n" + require(headers, "x-amz-date") +
           "\n" + info.date + "/" + info.region + "/" + info.service +
           "/aws4_request\n";
}

} // namespace

chunk_body_sha256::chunk_body_sha256(
    asio::ip::tcp::socket& s, const beast::flat_buffer& initial,
    chunked_body::trailing_headers trailing,
    const beast::http::request<beast::http::empty_body>& headers,
    const auth_info& info, std::string seed, std::string signing_key)
    : chunked_body(s, initial, trailing),
      m_signature_prelude(make_prelude(headers, info)),
      m_signing_key(std::move(signing_key)),
      m_string_to_sign(m_signature_prelude + std::move(seed) + "\n" +
                       SHA256_EMPTY_STRING + "\n") {
    LOG_DEBUG() << "seed: " << seed;
}

void chunk_body_sha256::on_chunk_header(const chunk_header& hdr) {
    if (auto it = hdr.extensions.find("chunk-signature");
        it != hdr.extensions.end()) {
        m_chunk_signature = it->second;
    }
}

void chunk_body_sha256::on_chunk_data(std::span<char> data) {
    m_hash.consume(data);
}

void chunk_body_sha256::on_chunk_done() {
    m_string_to_sign += m_hash.finalize();
    m_hash.reset();
    LOG_DEBUG() << "chunked string to sign: " << m_string_to_sign;

    auto signature =
        to_hex(hmac_sha256::from_string(m_signing_key, m_string_to_sign));
    if (signature != m_chunk_signature) {
        throw std::runtime_error("chunk signature mismatch: '" + signature +
                                 "' != '" + m_chunk_signature + "'");
    }

    m_string_to_sign =
        m_signature_prelude + signature + "\n" + SHA256_EMPTY_STRING + "\n";
}

void chunk_body_sha256::on_body_done() {
    m_string_to_sign += SHA256_EMPTY_STRING;
    LOG_DEBUG() << "final chunked string to sign: " << m_string_to_sign;
    auto signature =
        to_hex(hmac_sha256::from_string(m_signing_key, m_string_to_sign));
    if (signature != m_chunk_signature) {
        throw std::runtime_error("chunk signature mismatch: '" + signature +
                                 "' != '" + m_chunk_signature + "'");
    }
}

} // namespace uh::cluster::ep::http
