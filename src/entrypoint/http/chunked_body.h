#ifndef CORE_ENTRYPOINT_HTTP_CHUNKED_BODY_H
#define CORE_ENTRYPOINT_HTTP_CHUNKED_BODY_H

#include "body.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace uh::cluster::ep::http {

class chunked_body : public ep::http::body {
public:
    chunked_body(boost::asio::ip::tcp::socket& s,
                         const boost::beast::flat_buffer& initial);

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
};

} // namespace uh::cluster::ep::http

#endif
