#include "chunk_body_sha256.h"

#include "beast_utils.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::string make_prelude(partial_parse_result& req) {
    return "AWS4-HMAC-SHA256-PAYLOAD\n" + req.require("x-amz-date") + "\n" +
           req.auth->date + "/" + req.auth->region + "/" + req.auth->service +
           "/aws4_request\n";
}

} // namespace

chunk_body_sha256::chunk_body_sha256(partial_parse_result& req,
                                     chunked_body::trailing_headers trailing)
    : chunked_body(req, trailing),
      m_prelude(make_prelude(req)),
      m_signing_key(*req.auth->signing_key),
      m_to_sign(m_prelude + *req.auth->signature + "\n" + SHA256_EMPTY_STRING +
                "\n") {}

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
    m_to_sign += m_hash.finalize();
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
