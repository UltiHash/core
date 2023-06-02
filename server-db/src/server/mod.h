#ifndef SERVER_DATABASE_SERVER_MOD_H
#define SERVER_DATABASE_SERVER_MOD_H

#include <options/server_options.h>

#include <metrics/mod.h>
#include <storage/mod.h>

#include <memory>


namespace uh::dbn::server
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const net::server_config& options,
        dbn::storage::mod& storage,
        dbn::metrics::mod& metrics);

    ~mod();

    void start() const;
    void stop() const;

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::server

#endif
