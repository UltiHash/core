#ifndef SERVER_DATABASE_METRICS_MOD_H
#define SERVER_DATABASE_METRICS_MOD_H

#include <metrics/protocol_metrics.h>
#include <metrics/storage_metrics.h>

#include <memory>


namespace uh::dbn::metrics
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const uh::metrics::config& config);

    ~mod();

    uh::metrics::protocol_metrics& protocol();
    uh::dbn::metrics::storage_metrics& storage();
private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::metrics

#endif
