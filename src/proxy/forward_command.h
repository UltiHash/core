#pragma once

#include <entrypoint/commands/command.h>

namespace uh::cluster::proxy {

class forward_command : public command {
public:
    // FIXME decide on connection pool or replace by a single connection
    forward_command(connection_pool& pool) {}

    coro<ep::http::response> handle(ep::http::request&) override;
    coro<void> validate(const ep::http::request& req) override { co_return; }
    std::string action_id() const override;

private:
    connection_pool& m_pool;
};

} // namespace uh::cluster::proxy
