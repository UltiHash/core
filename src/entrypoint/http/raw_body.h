#ifndef CORE_ENTRYPOINT_HTTP_RAW_BODY_H
#define CORE_ENTRYPOINT_HTTP_RAW_BODY_H

#include "body.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace uh::cluster::ep::http {

class raw_body : public body {
public:
    raw_body(boost::asio::ip::tcp::socket& s,
             boost::beast::flat_buffer&& initial, std::size_t length);

    coro<std::size_t> read(std::span<char> dest) override;

private:
    boost::asio::ip::tcp::socket& m_socket;
    boost::beast::flat_buffer m_buffer;
    std::size_t m_length;
};

} // namespace uh::cluster::ep::http

#endif
