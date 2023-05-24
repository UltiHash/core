#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "allocation.h"
#include "common.h"
#include "protocol.h"
#include "serialization/serialization.h"
#include "protocol/request_interface.h"
#include <boost/iostreams/stream.hpp>


namespace uh::protocol
{

// ---------------------------------------------------------------------

enum class server_state
{
    disconnected,
    setup,
    normal,
};

// ---------------------------------------------------------------------

class server : public protocol
{
public:
    constexpr static std::size_t MINIMUM_CHUNK_SIZE = 64 * 1024;
    constexpr static std::size_t MAXIMUM_CHUNK_SIZE = 64 * 1024 * 1024;
    constexpr static std::size_t MAXIMUM_BLOCK_SIZE = 2u * 512 * 1024 * 1024;
    constexpr static std::size_t MAXIMUM_DATA_SIZE = 512lu * 1024lu * 1024lu;


    explicit server (const std::shared_ptr<net::socket>& client,
                     std::unique_ptr<uh::protocol::request_interface>&& handler_interface
                     ) : protocol (client), m_bs (*client), m_handler_interface(std::move(handler_interface)) {}
    virtual ~server() = default;

    void handle() override;

private:

    void handle_setup_request(uint8_t request_id);
    void handle_normal_request(uint8_t request_id);

    void handle_hello();
    void handle_quit();
    void handle_free_space();
    void handle_reset();
    void handle_client_statistics();
    void handle_write_chunks();
    void handle_read_chunks();

    server_state m_state = server_state::disconnected;
    serialization::buffered_serialization m_bs;

    std::unique_ptr<request_interface> m_handler_interface;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
