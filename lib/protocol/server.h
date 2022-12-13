#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "common.h"
#include "protocol.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

class server : public protocol
{
public:
    virtual ~server() = default;

    virtual server_information on_hello(const std::string& client_version) = 0;
    virtual blob on_write_chunk(blob&& data) = 0;
    virtual blob on_read_chunk(blob&& hash) = 0;

    virtual void handle(std::shared_ptr<net::socket> client) override;

    void handle_hello(std::iostream& io);
    void handle_write_chunk(std::iostream& io);
    void handle_read_chunk(std::iostream& io);
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
