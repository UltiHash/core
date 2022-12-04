#ifndef SERVER_DB_PROTOCOL_H
#define SERVER_DB_PROTOCOL_H

#include <protocol/server.h>
#include "storage_backend.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::server
{
public:
    protocol(storage_backend& storage);

    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual uh::protocol::blob on_write_chunk(uh::protocol::blob&& data) override;
    virtual uh::protocol::blob on_read_chunk(uh::protocol::blob&& hash) override;

private:
    storage_backend& m_storage;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
