#ifndef CORE_ENTRYPOINT_HTTP_RAW_BODY_H
#define CORE_ENTRYPOINT_HTTP_RAW_BODY_H

#include "beast_utils.h"
#include "body.h"

namespace uh::cluster::ep::http {

class raw_body : public body {
public:
    raw_body(partial_parse_result& req, std::size_t length);

    std::optional<std::size_t> length() const override;

    coro<std::size_t> read(std::span<char> dest) override;

private:
    boost::asio::ip::tcp::socket& m_socket;
    boost::beast::flat_buffer m_buffer;
    std::size_t m_length;
};

} // namespace uh::cluster::ep::http

#endif
