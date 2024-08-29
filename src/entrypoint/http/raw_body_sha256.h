#ifndef CORE_ENTRYPOINT_HTTP_RAW_BODY_SHA256_H
#define CORE_ENTRYPOINT_HTTP_RAW_BODY_SHA256_H

#include "raw_body.h"

#include "common/crypto/hash.h"

namespace uh::cluster::ep::http {

class raw_body_sha256 : public raw_body {
public:
    raw_body_sha256(partial_parse_result& req, std::size_t length);

    coro<std::size_t> read(std::span<char> dest) override;

private:
    std::string m_signature;
    sha256 m_hash;
};

} // namespace uh::cluster::ep::http

#endif
