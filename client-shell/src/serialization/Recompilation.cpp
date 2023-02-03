#include "Recompilation.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

Recompilation::Recompilation(const uh::client::option::client_config &config, std::unique_ptr<uh::protocol::client_pool>&& pool) : m_config(config), m_client_pool(std::move(pool))
{
    if (m_config.m_option == co::options_chosen::integrate)
        integrate();
    else if (m_config.m_option == co::options_chosen::retrieve)
        retrieve();
    else if (m_config.m_option == co::options_chosen::list)
        list();
}

// ---------------------------------------------------------------------

void Recompilation::integrate()
{
    //TODO:
}

// ---------------------------------------------------------------------

void Recompilation::retrieve()
{
    //TODO:
}

// ---------------------------------------------------------------------

void Recompilation::list()
{
    //TODO:
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

