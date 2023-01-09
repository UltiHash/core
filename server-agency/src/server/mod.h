#ifndef SERVER_AGENCY_SERVER_MOD_H
#define SERVER_AGENCY_SERVER_MOD_H

#include <config/options.h>
#include <metrics/metrics.h>

#include <memory>


namespace uh::an::server
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const config::options& options,
        an::metrics::metrics& metrics);

    ~mod();

    void start();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
