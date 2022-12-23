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

    std::streamsize write(std::span<const char> buffer);
    std::streamsize read(std::span<char> buffer);

    const boost::asio::ip::tcp::endpoint& peer() const;
    boost::asio::ip::tcp::endpoint& peer();

    bool valid() const;

protected:
    virtual std::streamsize write_impl(std::span<const char> buffer) = 0;
    virtual std::streamsize read_impl(std::span<char> buffer) = 0;

private:
    boost::asio::ip::tcp::endpoint m_peer;
    bool m_valid = true;
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
