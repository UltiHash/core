#pragma once

#include <common/utils/protocol_handler.h>
#include <proxy/request_factory.h>

namespace uh::cluster::proxy {

namespace http = uh::cluster::ep::http;

class handler : public protocol_handler {
public:
    explicit handler(std::unique_ptr<request_factory> factory,
                     std::function<std::unique_ptr<boost::asio::ip::tcp::socket>()> sf,
                     std::size_t buffer_size);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

    bool intercept(ep::http::raw_request& r) const;
    coro<void> handle(ep::http::stream& s, ep::http::raw_request& r);

private:
    std::unique_ptr<request_factory> m_factory;
    std::function<std::unique_ptr<boost::asio::ip::tcp::socket>()> m_sf;
    std::size_t m_buffer_size;

    coro<http::response> handle_request(boost::asio::ip::tcp::socket& s,
                                        http::raw_request& rawreq,
                                        const std::string& id,
                                        boost::beast::tcp_stream& ds);
};

} // namespace uh::cluster::proxy
