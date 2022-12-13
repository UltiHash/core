#ifndef UH_NET_SOCKET_H
#define UH_NET_SOCKET_H

#include <ios>
#include <span>

#include <boost/asio.hpp>
#include <boost/iostreams/categories.hpp>


namespace uh::net
{

// ---------------------------------------------------------------------

class socket
{
public:
    virtual ~socket() = default;

    virtual std::streamsize write(std::span<const char> buffer) = 0;
    virtual std::streamsize read(std::span<char> buffer) = 0;

    const boost::asio::ip::tcp::endpoint& peer() const;
    boost::asio::ip::tcp::endpoint& peer();

private:
    boost::asio::ip::tcp::endpoint m_peer;
};

// ---------------------------------------------------------------------

class socket_device
{
public:
    typedef char char_type;
    typedef boost::iostreams::bidirectional_device_tag category;

    socket_device(std::shared_ptr<socket> socket);

    std::streamsize write(const char* s, std::streamsize n);
    std::streamsize read(char*s, std::streamsize n);

private:
    std::shared_ptr<socket> m_socket;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
