#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "common.h"
#include "protocol.h"

#include <boost/iostreams/stream.hpp>


namespace uh::protocol
{

// ---------------------------------------------------------------------

enum class server_state
{
    disconnected,
    setup,
    normal,
    reading,
};

// ---------------------------------------------------------------------

using iostream = boost::iostreams::stream<net::socket_device>;

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

    void handle_setup_request(iostream& io, uint8_t request_id);
    void handle_normal_request(iostream& io, uint8_t request_id);
    void handle_reading_request(iostream& io, uint8_t request_id);

    void handle_hello(iostream& io);
    void handle_write_block(iostream& io);
    void handle_read_block(iostream& io);
    void handle_quit(iostream& io);
    void handle_free_space(iostream& io);

private:
    server_state m_state = server_state::disconnected;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
