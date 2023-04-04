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

class protocol : public uh::protocol::handler_interface
{
public:
    explicit protocol(cluster::mod& cluster);

    uh::protocol::server_information on_hello(const std::string& client_version) override;
    std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) override;
    std::unique_ptr<uh::protocol::allocation> on_allocate_chunk(std::size_t size) override;

    std::size_t on_free_space() override;
    void on_quit(const std::string& reason) override;
    void on_reset() override;

    std::size_t on_next_chunk(std::span<char> buffer) override;
    void on_finalize() override;
    void on_write_chunk(std::span<char> buffer) override;

private:
    cluster::mod& m_cluster;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
