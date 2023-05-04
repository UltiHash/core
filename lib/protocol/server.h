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
    reading,
    writing,
};

// ---------------------------------------------------------------------

class server : public protocol
{
public:
    constexpr static std::size_t MINIMUM_CHUNK_SIZE = 64 * 1024;
    constexpr static std::size_t MAXIMUM_CHUNK_SIZE = 64 * 1024 * 1024;
    constexpr static std::size_t SMALL_CHUNK_LIMIT = 64 * 1024 * 1024;
    constexpr static std::size_t XSMALL_CHUNK_SIZE_LIMIT = std::numeric_limits <std::uint16_t>::max();
    constexpr static std::size_t XSMALL_CHUNK_COUNT_LIMIT = std::numeric_limits <std::uint16_t>::max();



    constexpr static std::size_t MAXIMUM_BLOCK_SIZE = 2u * 1024 * 1024 * 1024;

    explicit server (const std::shared_ptr<net::socket>& client,
                     std::unique_ptr<uh::protocol::request_interface>&& handler_interface
                     ) : protocol (client), m_bs (*client), m_handler_interface(std::move(handler_interface)) {}
    virtual ~server() = default;

    void handle() override;

private:

    void handle_setup_request(uint8_t request_id);
    void handle_normal_request(uint8_t request_id);
    void handle_reading_request(uint8_t request_id);
    void handle_writing_request(uint8_t request_id);

    void handle_hello();
    void handle_read_block();
    void handle_quit();
    void handle_free_space();
    void handle_reset();
    void handle_next_chunk();
    void handle_allocate_chunk();
    void handle_write_small_block();
    void handle_read_small_block();
    void handle_write_xsmall_blocks();
    void handle_write_chunk();
    void handle_finalize_block();
    void handle_client_statistics();

    server_state m_state = server_state::disconnected;

    std::unique_ptr<io::device> m_read_block;        // invariant: (!m_read_block) == (m_state != reading)
    std::unique_ptr<allocation> m_write_alloc;        // invariant: (!m_write_alloc) == (m_state != writing)
    serialization::buffered_serialization m_bs;

    std::unique_ptr<request_interface> m_handler_interface;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
