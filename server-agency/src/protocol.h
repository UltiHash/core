#ifndef SERVER_AGENCY_PROTOCOL_H
#define SERVER_AGENCY_PROTOCOL_H

#include <protocol/server.h>
#include <boost/asio.hpp>

#include <memory>


using namespace boost::asio::ip;

namespace uh::an
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::server
{
public:
    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual uh::protocol::blob on_write_chunk(uh::protocol::blob&& data) override;
    virtual uh::protocol::blob on_read_chunk(uh::protocol::blob&& hash) override;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
