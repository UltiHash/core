#include <state/mod.h>
#include <logging/logging_boost.h>

namespace uh::an::state
{

// ---------------------------------------------------------------------

mod::mod(const storage_config& config) : m_client_metrics_state(std::make_unique<client_metrics_state>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting persistence module";
    m_client_metrics_state->start();
}

// ---------------------------------------------------------------------

void mod::stop()
{
    INFO << "stopping persistence module";
    m_client_metrics_state->stop();
}

// ---------------------------------------------------------------------

client_metrics_state& mod::client_metrics()
{
    return *m_client_metrics_state;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
