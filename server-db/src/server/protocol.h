#ifndef SERVER_DATABASE_SERVER_PROTOCOL_H
#define SERVER_DATABASE_SERVER_PROTOCOL_H

#include <storage/mod.h>
#include <protocol/server.h>

#include <memory>


using namespace boost::asio::ip;

namespace uh::dbn::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::server
{
public:
    protocol(storage::mod& storage);

    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual uh::protocol::blob on_write_chunk(uh::protocol::blob&& data) override;
    virtual uh::protocol::blob on_read_chunk(uh::protocol::blob&& hash) override;
    virtual std::size_t on_free_space() override;

private:
    storage::mod& m_storage;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::server

#endif
