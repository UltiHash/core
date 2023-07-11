#ifndef SERVER_DATABASE_SERVER_PROTOCOL_H
#define SERVER_DATABASE_SERVER_PROTOCOL_H

#include <net/server_info.h>
#include <protocol/request_interface.h>

#include <storage/mod.h>

#include <memory>
#include <shared_mutex>


namespace uh::dbn::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::request_interface
{
public:
    explicit protocol(storage::backend& storage, const uh::net::server_info& serv_info);

    uh::protocol::server_information on_hello(const std::string& client_version) override;
    std::size_t on_free_space() override;
    uh::protocol::write_chunks::response on_write_chunks (const uh::protocol::write_chunks::request&) override;
    uh::protocol::read_chunks::response on_read_chunks (const uh::protocol::read_chunks::request&) override;

    uh::protocol::write_key_value::response on_write_kv (const uh::protocol::write_key_value::request &) override;
    uh::protocol::read_key_value::response on_read_kv (const uh::protocol::read_key_value::request &) override;
private:
    storage::backend& m_storage;
    const uh::net::server_info &m_serv_info;
    std::shared_mutex m;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::server

#endif
