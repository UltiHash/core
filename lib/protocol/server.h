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
    virtual blob on_write_block(blob&& data) = 0;
    virtual blob on_read_block(blob&& hash) = 0;
    virtual std::size_t on_free_space();

    virtual void on_quit(const std::string& reason);

    virtual void handle(std::shared_ptr<net::socket> client) override;

    void handle_hello(std::iostream& io);
    void handle_write_block(std::iostream& io);
    void handle_read_block(std::iostream& io);
    void handle_quit(std::iostream& io);
    void handle_free_space(std::iostream& io);
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
