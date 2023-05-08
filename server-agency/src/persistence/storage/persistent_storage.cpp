#include "persistent_storage.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

persistent_storage::persistent_storage(const persistence_config& config) : m_config(config)
{
}

// ---------------------------------------------------------------------

void persistent_storage::start()
{
    INFO << "Persistence Path: " << m_config.persistence_path ;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
