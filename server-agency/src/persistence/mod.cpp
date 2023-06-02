#include <persistence/mod.h>
#include <logging/logging_boost.h>
#include "persistence/storage/client_metrics_persistence.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

mod::mod(const uh::options::persistence_config& config) : m_storage(std::make_unique<client_metrics>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "       starting persistence module";
    m_storage->start();
}

// ---------------------------------------------------------------------

void mod::stop()
{
    INFO << "         stopping persistence module";
    m_storage->stop();
}

// ---------------------------------------------------------------------

client_metrics& mod::clientM_persistence()
{
    return *m_storage;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
