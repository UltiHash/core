#ifndef SERVER_AGENCY_SERVER_H
#define SERVER_AGENCY_SERVER_H

#include "connection.h"
#include "protocol.h"
#include "scheduler.h"

#include <utils/factory.h>

#include <boost/asio.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>


namespace uh::an
{

// ---------------------------------------------------------------------

struct server_config
{
    uint16_t port = 0x5548;

    std::filesystem::path tls_chain;
    std::filesystem::path tls_pkey;

    std::size_t threads = 3;
};

// TODO run multiple threads on m_context ( see https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/reference/io_context/run/overload1.html)
// TODO spawn handlers

// ---------------------------------------------------------------------

class server
{
public:
    server(const server_config& config,
           const util::factory<protocol>& protocol_factory);

    void run();

    void spawn_client(std::shared_ptr<connection> client);

private:
    boost::asio::io_context m_context;
    boost::asio::ip::tcp::acceptor m_acceptor;

    const util::factory<protocol>& m_protocol_factory;
    scheduler m_scheduler;

    std::atomic<bool> m_running;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
