#include <persistence/mod.h>
#include "persistence/storage/persistent_storage.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

mod::mod(const persistence_config& config) : m_storage(std::make_unique<persistent_storage>(config))
{
}

// ---------------------------------------------------------------------

storage& mod::storage()
{
    return *m_storage;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
