#pragma once

#include "raw_body.h"

#include <common/crypto/hash.h>

namespace uh::cluster::ep::http {

class raw_body_sha256 : public raw_body {
public:
    raw_body_sha256(stream& s, raw_request& req, std::string signature);

    coro<std::span<const char>> read(std::size_t count) override;

private:
    std::string m_signature;
    sha256 m_hash;
};

} // namespace uh::cluster::ep::http
