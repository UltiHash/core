#include "chunk_body_sha256.h"

#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::string make_prelude(raw_request& req, const aws4_signature_info& info) {
    return "AWS4-HMAC-SHA256-PAYLOAD\n" + req.require("x-amz-date") + "\n" +
           std::string(info.date) + "/" + std::string(info.region) + "/" +
           std::string(info.service) + "/aws4_request\n";
}

} // namespace

chunk_body_sha256::chunk_body_sha256(stream& s,
                                     raw_request& req,
                                     const aws4_signature_info& info,
                                     const std::string& signing_key,
                                     const std::string& signature,
                                     chunked_body::trailing_headers trailing)
    : chunked_body(s, trailing),
      m_prelude(make_prelude(req, info)),
      m_signing_key(signing_key),
      m_to_sign(m_prelude + signature + "\n" + SHA256_EMPTY_STRING + "\n") {}

void chunk_body_sha256::on_chunk_header(const chunk_header& hdr) {
    if (auto it = hdr.extensions.find("chunk-signature");
        it != hdr.extensions.end()) {
        m_chunk_signature = it->second;
    }
}

void chunk_body_sha256::on_chunk_data(std::span<const char> data) {
    m_hash.consume(data);
}

void chunk_body_sha256::on_chunk_done() {
    m_to_sign += to_hex(m_hash.finalize());
    m_hash.reset();

    auto signature = to_hex(hmac_sha256::from_string(m_signing_key, m_to_sign));
    if (signature != m_chunk_signature) {
        throw std::runtime_error("chunk signature mismatch");
    }

    m_to_sign = m_prelude + signature + "\n" + SHA256_EMPTY_STRING + "\n";
}

void chunk_body_sha256::on_body_done() {
    m_to_sign += SHA256_EMPTY_STRING;
    auto signature = to_hex(hmac_sha256::from_string(m_signing_key, m_to_sign));
    if (signature != m_chunk_signature) {
        throw std::runtime_error("chunk signature mismatch");
    }
}

} // namespace uh::cluster::ep::http
