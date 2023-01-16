#ifndef UH_NET_PLAIN_SOCKET_H
#define UH_NET_PLAIN_SOCKET_H

#include <net/socket.h>
#include <util/factory.h>

#include <boost/asio.hpp>


namespace uh::net
{

// ---------------------------------------------------------------------

class plain_socket : public socket
{
public:
    plain_socket(boost::asio::ip::tcp::socket&& sock);

    static std::unique_ptr<plain_socket> connect(
        boost::asio::io_context& ctx,
        const std::string& hostname,
        uint16_t port);

protected:
    virtual std::streamsize write_impl(std::span<const char> buffer) override;
    virtual std::streamsize read_impl(std::span<char> buffer) override;

private:
    boost::asio::ip::tcp::socket m_sock;
};

// ---------------------------------------------------------------------

class plain_socket_factory : public util::factory<socket>
{
public:
    plain_socket_factory(
        boost::asio::io_context& ctx,
        const std::string& hostname,
        uint16_t port);

    virtual std::unique_ptr<socket> create() override;

private:
    boost::asio::io_context& m_ctx;
    std::string m_hostname;
    uint16_t m_port;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
