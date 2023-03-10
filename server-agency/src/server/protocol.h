#ifndef SERVER_AGENCY_SERVER_PROTOCOL_H
#define SERVER_AGENCY_SERVER_PROTOCOL_H

#include <cluster/mod.h>
#include <protocol/client_pool.h>
#include <protocol/server.h>
#include <boost/asio.hpp>

#include <memory>


using namespace boost::asio::ip;

namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::server
{
public:
    protocol(cluster::mod& cluster);

    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual uh::protocol::block_meta_data on_write_block(uh::protocol::blob&& data) override;
    virtual std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) override;

private:
    cluster::mod& m_cluster;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
