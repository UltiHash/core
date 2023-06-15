#include <state/mod.h>
#include <logging/logging_boost.h>

namespace uh::an::state
{

// ---------------------------------------------------------------------

mod::mod(const storage_config& config) : m_client_metrics(std::make_unique<client_metrics>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting persistence module";
    m_client_metrics->start();
}

// ---------------------------------------------------------------------

void mod::stop()
{
    INFO << "stopping persistence module";
    m_client_metrics->stop();
}

// ---------------------------------------------------------------------

client_metrics& mod::client_metrics_state()
{
    return *m_client_metrics;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
