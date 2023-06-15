#include "mod.h"
#include <logging/logging_boost.h>

namespace uh::dbn::state
{

// ---------------------------------------------------------------------

mod::mod(const uh::dbn::storage::storage_config& config) : m_scheduled_compressions_state(std::make_unique<scheduled_compressions_state>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting persistence module";
    m_scheduled_compressions_state->start();
}

// ---------------------------------------------------------------------

scheduled_compressions_state& mod::scheduled_compressions()
{
    return *m_scheduled_compressions_state;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence
