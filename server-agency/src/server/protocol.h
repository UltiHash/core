#ifndef SERVER_AGENCY_SERVER_PROTOCOL_H
#define SERVER_AGENCY_SERVER_PROTOCOL_H

#include <cluster/mod.h>
#include <protocol/protocol.h>
#include <protocol/request_interface.h>
#include <memory>

#include <net/server_info.h>

using namespace boost::asio::ip;

namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::request_interface
{
public:
    explicit protocol(cluster::mod& cluster, const uh::net::server_info &serv_info);

    uh::protocol::server_information on_hello(const std::string& client_version) override;
    std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) override;
    std::unique_ptr<uh::protocol::allocation> on_allocate_chunk(std::size_t size) override;
    uh::protocol::block_meta_data on_write_small_block (std::span <char> buffer) override;
    void on_client_statistics(uh::protocol::client_statistics::request& client_stat) override;
    std::size_t on_free_space() override;
    void on_next_chunk(std::span<char> buffer) override;

private:
    cluster::mod& m_cluster;
    const uh::net::server_info &m_serv_info;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
