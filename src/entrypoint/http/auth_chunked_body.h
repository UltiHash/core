#ifndef CORE_ENTRYPOINT_HTTP_CHUNKED_BODY_H
#define CORE_ENTRYPOINT_HTTP_CHUNKED_BODY_H

#include "body.h"
#include "common/crypto/hash.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace uh::cluster::ep::http {

class auth_chunked_body : public ep::http::body {
public:
    /**
     * @param prelude ::= algorithm + "\n" + X-AMZ-date + "\n" + scope
     *   algorithm ::= "AWS-HMAC-SHA256-PAYLOAD"
     *   scope ::= credential from authorization header
     * @param seed seed signature derived from header
     * @param signing_key signing key
     */
    auth_chunked_body(boost::asio::ip::tcp::socket& s,
                      const boost::beast::flat_buffer& initial,
                      const std::string& prelude, const std::string& seed,
                      const std::string& signing_key);

    coro<std::size_t> read(std::span<char> dest) override;

private:
    coro<void> read_nl();
    std::size_t find_nl() const;
    coro<std::size_t> read_chunk_header();
    coro<std::size_t> read_data(std::span<char> buffer);

    static constexpr std::size_t BUFFER_SIZE = MEBI_BYTE;

    boost::asio::ip::tcp::socket& m_socket;
    std::vector<char> m_buffer;
    std::size_t m_chunk_size = 0ull;
    bool m_end = false;

    sha256 m_hash;
    std::string m_signature_prelude;
    std::string m_signing_key;
    std::string m_chunk_signature;
    std::string m_string_to_sign;
};

} // namespace uh::cluster::ep::http

#endif
