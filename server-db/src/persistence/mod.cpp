#include "mod.h"
#include <logging/logging_boost.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

mod::mod(const persistence_config& config) : m_jq_persistence(std::make_unique<job_queue_persistence>(config))
{
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "       starting persistence module";
    m_jq_persistence->start();
}

// ---------------------------------------------------------------------

job_queue_persistence& mod::jobQ_persistence()
{
    return *m_jq_persistence;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence
