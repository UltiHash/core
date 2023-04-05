#ifndef SERVER_AGENCY_SERVER_PROTOCOL_H
#define SERVER_AGENCY_SERVER_PROTOCOL_H

#include <cluster/mod.h>
#include <protocol/protocol.h>
#include <protocol/request_interface.h>
#include <memory>

using namespace boost::asio::ip;

namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::request_interface
{
public:
    explicit protocol(cluster::mod& cluster);

    uh::protocol::server_information on_hello(const std::string& client_version) override;
    std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) override;
    std::unique_ptr<uh::protocol::allocation> on_allocate_chunk(std::size_t size) override;
    std::size_t on_free_space() override;
    std::size_t on_next_chunk(std::span<char> buffer) override;

private:
    cluster::mod& m_cluster;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
