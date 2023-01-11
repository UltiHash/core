#ifndef UH_NET_SOCKET_H
#define UH_NET_SOCKET_H

#include <io/device.h>

#include <boost/asio.hpp>


namespace uh::net
{

// ---------------------------------------------------------------------

class socket : public io::device
{
public:
    virtual ~socket() = default;

    virtual std::streamsize write(std::span<const char> buffer) override;
    virtual std::streamsize read(std::span<char> buffer) override;

    const boost::asio::ip::tcp::endpoint& peer() const;
    boost::asio::ip::tcp::endpoint& peer();

    virtual bool valid() const override;

protected:
    virtual std::streamsize write_impl(std::span<const char> buffer) = 0;
    virtual std::streamsize read_impl(std::span<char> buffer) = 0;

private:
    boost::asio::ip::tcp::endpoint m_peer;
    bool m_valid = true;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
