#pragma once

#include <proxy/request_factory.h>
#include <proxy/command_factory.h>

namespace uh::cluster::proxy {

namespace http = uh::cluster::ep::http;

class handler : public protocol_handler {
public:
    explicit handler(std::unique_ptr<request_factory> factory,
                     std::function<std::unique_ptr<boost::beast::tcp_stream>()> sf,
                     std::size_t buffer_size);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    std::unique_ptr<request_factory> m_factory;
    std::function<std::unique_ptr<boost::beast::tcp_stream>()> m_sf;
    std::size_t m_buffer_size;

    coro<http::response> handle_request(boost::asio::ip::tcp::socket& s,
                                        http::raw_request& rawreq,
                                        const std::string& id,
                                        boost::beast::tcp_stream& ds);
};

} // namespace uh::cluster::proxy
