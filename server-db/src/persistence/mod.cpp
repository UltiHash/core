#include "mod.h"
#include <logging/logging_boost.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

mod::mod(const uh::dbn::storage::storage_config& config) : m_scheduling_persistence(std::make_unique<scheduled_compressions_persistence>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting persistence module";
    m_scheduling_persistence->start();
}

// ---------------------------------------------------------------------

scheduled_compressions_persistence& mod::scheduled_persistence()
{
    return *m_scheduling_persistence;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence
