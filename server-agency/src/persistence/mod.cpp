#include <persistence/mod.h>
#include "persistence/storage/client_metrics_storage.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

mod::mod(const persistence_config& config) : m_storage(std::make_unique<client_metrics>(config)),
                                             m_config(config)
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "       starting persistence module";
    m_storage->start();
}

// ---------------------------------------------------------------------

client_metrics& mod::storage()
{
    return *m_storage;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
