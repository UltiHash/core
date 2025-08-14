#pragma once

#include "chunked_body.h"
#include <common/crypto/hash.h>
#include <entrypoint/http/aws4_hmac_sha256.h>

namespace uh::cluster::ep::http {

class chunk_body_sha256 : public chunked_body {
public:
    chunk_body_sha256(stream& s, raw_request& req,
                      const aws4_signature_info& info,
                      const std::string& signing_key,
                      const std::string& signature,
                      chunked_body::trailing_headers trailing);

    void on_chunk_header(const chunk_header&) override;
    void on_chunk_data(std::span<const char>) override;
    void on_chunk_done() override;
    void on_body_done() override;

private:
    sha256 m_hash;
    std::string m_prelude;
    std::string m_signing_key;
    std::string m_chunk_signature;
    std::string m_to_sign;
};

} // namespace uh::cluster::ep::http
