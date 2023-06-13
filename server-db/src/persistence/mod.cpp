#include "mod.h"
#include <logging/logging_boost.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

mod::mod(const uh::options::persistence_config& config) : m_scheduling_persistence(std::make_unique<scheduled_compressions_persistence>(config)),
        m_identity_persistence(std::make_unique<uuid_persistence>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "       starting state persistence modules";
    m_scheduling_persistence->start();
    m_identity_persistence->start();
}

// ---------------------------------------------------------------------

scheduled_compressions_persistence& mod::scheduled_persistence()
{
    return *m_scheduling_persistence;
}

// ---------------------------------------------------------------------

uuid_persistence& mod::identity_persistence()
{
    return *m_identity_persistence;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence
