#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include <protocol/allocation.h>
#include <protocol/common.h>
#include <protocol/protocol.h>
#include <protocol/request_interface.h>

#include <serialization/serialization.h>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class server : public protocol
{
public:
    constexpr static std::size_t MAXIMUM_DATA_SIZE = 512lu * 1024lu * 1024lu;

    explicit server(const std::shared_ptr<net::socket>& client,
                    std::unique_ptr<request_interface>&& handler_interface);

    void handle() override;

private:
    void handle_normal_request(uint8_t request_id);

    void handle_hello();
    void handle_quit();
    void handle_free_space();
    void handle_client_statistics();
    void handle_write_chunks();
    void handle_read_chunks();
    void handle_write_kv();
    void handle_read_kv();

    std::shared_ptr<net::socket> m_client;
    serialization::buffered_serialization m_bs;
    std::unique_ptr<request_interface> m_handler_interface;

};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
