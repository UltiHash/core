#pragma once

#include "body.h"
#include "raw_request.h"

namespace uh::cluster::ep::http {

class raw_body : public body {
public:
    raw_body(boost::asio::ip::tcp::socket& sock, raw_request& req);
    raw_body(boost::asio::ip::tcp::socket& sock, std::vector<char> b, std::size_t len);
    raw_body(boost::asio::ip::tcp::socket& sock, boost::beast::flat_buffer b, std::size_t len);

    std::optional<std::size_t> length() const override;

    coro<std::size_t> read(std::span<char> dest) override;

    std::vector<boost::asio::const_buffer>
    get_raw_buffer() const override {
        return m_raw_buffers;
    }

private:
    boost::asio::ip::tcp::socket& m_socket;
    std::vector<char> m_body_prefix;
    std::size_t m_length;
    std::size_t m_read_position = 0;
    std::vector<boost::asio::const_buffer> m_raw_buffers;
};

} // namespace uh::cluster::ep::http
