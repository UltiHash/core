#ifndef NET_SERVER_H
#define NET_SERVER_H

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
    constexpr static std::size_t DEFAULT_THREADS = 3;

    uint16_t port = DEFAULT_PORT;

    std::filesystem::path tls_chain;
    std::filesystem::path tls_pkey;

    std::size_t threads = DEFAULT_THREADS;
};

// ---------------------------------------------------------------------

class server
{
public:
    server(const server_config& config,
           const util::factory<uh::protocol::protocol>& protocol_factory);

    void run();

    void spawn_client(std::shared_ptr<connection> client);

private:
    boost::asio::io_context m_context;
    boost::asio::ip::tcp::acceptor m_acceptor;

    const util::factory<uh::protocol::protocol>& m_protocol_factory;
    scheduler m_scheduler;

    std::atomic<bool> m_running;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
