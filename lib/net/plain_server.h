#ifndef UH_NET_PLAIN_SERVER_H
#define UH_NET_PLAIN_SERVER_H

#include <net/server.h>
#include <net/scheduler.h>
#include <protocol/protocol.h>
#include <util/factory.h>

#include <boost/asio.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>

namespace uh::net
{

// ---------------------------------------------------------------------

struct server_config
{
    constexpr static uint16_t DEFAULT_PORT = 0x5548;
    constexpr static std::size_t DEFAULT_THREADS = 100;

    uint16_t port = DEFAULT_PORT;

    std::filesystem::path tls_chain;
    std::filesystem::path tls_pkey;

    std::size_t threads = DEFAULT_THREADS;
};

// ---------------------------------------------------------------------

class plain_server : public server
{
public:
    plain_server(const server_config& config,
                 uh::protocol::protocol_factory& protocol_factory);

    void run() override;

    void spawn_client(const std::shared_ptr<socket>& client);

private:
    boost::asio::io_context m_context;
    boost::asio::ip::tcp::acceptor m_acceptor;

    uh::protocol::protocol_factory& m_protocol_factory;
    scheduler m_scheduler;

    std::atomic<bool> m_running;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
