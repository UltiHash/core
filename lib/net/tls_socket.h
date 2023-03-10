#ifndef UH_NET_TLS_SOCKET_H
#define UH_NET_TLS_SOCKET_H

#include <net/socket.h>
#include <util/factory.h>

#include <boost/asio/ssl.hpp>


namespace uh::net
{

// ---------------------------------------------------------------------

class tls_socket : public socket
{
public:
    explicit tls_socket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& sock);

    static std::unique_ptr<tls_socket> connect(
        boost::asio::io_context& ctx,
        boost::asio::ssl::context& ssl,
        const std::string& hostname,
        uint16_t port);

protected:
    virtual std::streamsize write_impl(std::span<const char> buffer) override;
    virtual std::streamsize read_impl(std::span<char> buffer) override;

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_sock;
};

// ---------------------------------------------------------------------

class tls_socket_factory : public util::factory<socket>
{
public:
    tls_socket_factory(
        boost::asio::io_context& ctx,
        boost::asio::ssl::context& ssl,
        const std::string& hostname,
        uint16_t port);

    std::unique_ptr<socket> create() override;

private:
    boost::asio::io_context& m_ctx;
    boost::asio::ssl::context& m_ssl;
    std::string m_hostname;
    uint16_t m_port;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
