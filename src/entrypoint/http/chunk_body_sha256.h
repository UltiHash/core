#ifndef CORE_ENTRYPOINT_HTTP_CHUNKED_BODY_SHA256_H
#define CORE_ENTRYPOINT_HTTP_CHUNKED_BODY_SHA256_H

#include "beast_utils.h"
#include "chunked_body.h"
#include "common/crypto/hash.h"

namespace uh::cluster::ep::http {

class chunk_body_sha256 : public chunked_body {
public:
    /**
     * @param algorithm AWS4-HMAC-SHA256
     * @param prelude ::= X-AMZ-date + "\n" + scope
     *   scope ::= credential from authorization header
     * @param seed seed signature derived from header
     * @param signing_key signing key
     */
    chunk_body_sha256(partial_parse_result& req,
                      chunked_body::trailing_headers trailing);

    void on_chunk_header(const chunk_header&) override;
    void on_chunk_data(std::span<char>) override;
    void on_chunk_done() override;
    void on_body_done() override;

private:
    sha256 m_hash;
    std::string m_algorithm;
    std::string m_signature_prelude;
    std::string m_signing_key;
    std::string m_chunk_signature;
    std::string m_string_to_sign;
};

} // namespace uh::cluster::ep::http

#endif
