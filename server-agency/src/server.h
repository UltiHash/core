#ifndef SERVER_AGENCY_SERVER_H
#define SERVER_AGENCY_SERVER_H

#include "connection.h"
#include "protocol.h"
#include "scheduler.h"

#include <options/basic_options.h>
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
    constexpr static uint16_t DEFAULT_PORT = 0x5548;
    constexpr static std::size_t DEFAULT_THREADS = 3;

    uint16_t port = DEFAULT_PORT;

    std::filesystem::path tls_chain;
    std::filesystem::path tls_pkey;

    std::size_t threads = DEFAULT_THREADS;
};

// ---------------------------------------------------------------------

class server_options : public uh::options::options
{
public:
    server_options();

    void apply(options& opts);
    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    const server_config& config() const;

private:
    server_config m_config;
    boost::program_options::options_description m_desc;
};

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
