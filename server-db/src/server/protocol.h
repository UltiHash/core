#ifndef SERVER_DATABASE_SERVER_PROTOCOL_H
#define SERVER_DATABASE_SERVER_PROTOCOL_H

#include <storage/mod.h>
#include <protocol/request_interface.h>

#include <memory>
#include <net/server_info.h>


using namespace boost::asio::ip;

namespace uh::dbn::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::request_interface
{
public:
    explicit protocol(storage::backend& storage, const uh::net::server_info &serv_info);

    uh::protocol::server_information on_hello(const std::string& client_version) override;
    std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) override;
    std::size_t on_free_space() override;
    std::unique_ptr<uh::protocol::allocation> on_allocate_chunk(std::size_t size) override;
    void on_next_chunk(std::span<char> buffer) override;
    uh::protocol::write_chunks::response on_write_chunks (const uh::protocol::write_chunks::request &) override;
    uh::protocol::read_chunks::response on_read_chunks (const uh::protocol::read_chunks::request &) override;

private:
    storage::backend& m_storage;
    const uh::net::server_info &m_serv_info;
    std::vector <char> m_read_buffer;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::server

#endif
