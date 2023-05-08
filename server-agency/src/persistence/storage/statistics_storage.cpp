#include "statistics_storage.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

statistics_storage::statistics_storage(const persistence_config& config) :
    m_path(config.persistence_path / std::filesystem::path("client_statistics"))
{
}

// ---------------------------------------------------------------------

void statistics_storage::start()
{
    if (std::filesystem::is_directory(m_path))
        throw std::runtime_error("Directory not found: " + m_path.string());

    INFO << "Client Statistics Storage Directory: " << m_path ;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
