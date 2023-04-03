#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "allocation.h"
#include "common.h"
#include "protocol.h"
#include "serialization/serialization.h"
#include "net/scheduler.h"

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

using iostream = boost::iostreams::stream<io::boost_device>;

// ---------------------------------------------------------------------

class server : public protocol
{
public:
    constexpr static std::size_t MINIMUM_CHUNK_SIZE = 64 * 1024;
    constexpr static std::size_t MAXIMUM_CHUNK_SIZE = 64 * 1024 * 1024;

    constexpr static std::size_t MAXIMUM_BLOCK_SIZE = 2u * 1024 * 1024 * 1024;

    explicit server (const std::shared_ptr<net::socket>& client): protocol(client), m_bs (*client) {}
    virtual ~server() = default;

    virtual server_information on_hello(const std::string& client_version) = 0;
    virtual std::unique_ptr<io::device> on_read_block(blob&& hash) = 0;
    virtual std::size_t on_free_space();

    virtual void on_quit(const std::string& reason);
    virtual void on_reset();
    virtual std::size_t on_next_chunk(std::span<char> buffer);
    virtual void on_finalize();
    virtual void on_write_chunk(std::span<char> buffer);
    virtual std::unique_ptr<allocation> on_allocate_chunk(std::size_t size) = 0;

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
    void handle_write_chunk();
    void handle_finalize_block();

    server_state m_state = server_state::disconnected;

    std::unique_ptr<io::device> m_read_block;        // invariant: (!m_read_block) == (m_state != reading)
    std::unique_ptr<allocation> m_write_alloc;        // invariant: (!m_write_alloc) == (m_state != writing)
    serialization::buffered_serialization m_bs;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
