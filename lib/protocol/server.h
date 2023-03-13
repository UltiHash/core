#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "common.h"
#include "protocol.h"
#include "serialization/serialization.h"
#include "exception.h"
#include "messages.h"

#include <logging/logging_boost.h>
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

using iostream = boost::iostreams::stream<io::boost_device>;

// ---------------------------------------------------------------------

class server : public protocol
{
public:


    constexpr static std::size_t MINIMUM_CHUNK_SIZE = 64 * 1024;
    constexpr static std::size_t MAXIMUM_CHUNK_SIZE = 64 * 1024 * 1024;

    explicit server (std::shared_ptr<net::socket> client): protocol (client), bs (*client) {}
    virtual ~server() = default;

    virtual server_information on_hello(const std::string& client_version) = 0;
    virtual block_meta_data on_write_block(blob&& data) = 0;
    virtual std::unique_ptr<io::device> on_read_block(blob&& hash) = 0;
    virtual std::size_t on_free_space();

    virtual void on_quit(const std::string& reason);
    virtual void on_reset();
    virtual std::size_t on_next_chunk(std::span<char> buffer);

    virtual void handle() override;


private:
    server_state m_state = server_state::disconnected;
    std::unique_ptr<io::device> m_block;        // invariant: (!m_block) == (m_state != reading)
    serialization::buffered_serialization bs;


    void handle_setup_request(uint8_t request_id);
    void handle_normal_request(uint8_t request_id);
    void handle_reading_request(uint8_t request_id);

    void handle_hello();
    void handle_write_block();
    void handle_read_block();
    void handle_quit();
    void handle_free_space();
    void handle_reset();
    void handle_next_chunk();

};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
