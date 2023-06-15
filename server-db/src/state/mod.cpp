#include "mod.h"
#include <logging/logging_boost.h>

namespace uh::dbn::state
{

// ---------------------------------------------------------------------

mod::mod(const uh::dbn::storage::storage_config& config) : m_scheduling(std::make_unique<scheduled_compressions>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting persistence module";
    m_scheduling->start();
}

// ---------------------------------------------------------------------

scheduled_compressions& mod::scheduled_storage()
{
    return *m_scheduling;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence
