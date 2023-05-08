#include <persistence/mod.h>
#include "persistence/storage/statistics_storage.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

mod::mod(const persistence_config& config) : m_storage(std::make_unique<statistics_storage>(config))
{
}

// ---------------------------------------------------------------------

storage& mod::client_storage()
{
    return *m_storage;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
