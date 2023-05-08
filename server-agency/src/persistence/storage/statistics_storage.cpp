#include "statistics_storage.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

statistics_storage::statistics_storage(const persistence_config& config) :
    m_target_path(config.persistence_path / std::filesystem::path("client_statistics"))
{
}

// ---------------------------------------------------------------------

void statistics_storage::start()
{
    if (std::filesystem::is_directory(m_target_path))
        throw std::runtime_error("Directory not found: " + m_target_path.string());

    INFO << "client statistics storage directory: " << m_target_path;
}

// ---------------------------------------------------------------------

void serialize()
{

}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
