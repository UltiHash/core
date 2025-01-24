#pragma once

#include "raw_body.h"

#include "common/crypto/hash.h"

namespace uh::cluster::ep::http {

class raw_body_sha256 : public raw_body {
public:
    raw_body_sha256(boost::asio::ip::tcp::socket& sock, raw_request& req,
                    std::string signature);

    coro<std::size_t> read(std::span<char> dest) override;

private:
    std::string m_signature;
    sha256 m_hash;
};

} // namespace uh::cluster::ep::http
