#include "client_metrics_storage.h"
#include <persistence/options.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

client_metrics::client_metrics(const persistence_config& config) :
    m_target_path(config.persistence_path / std::filesystem::path("client_metrics"))
{
}

// ---------------------------------------------------------------------

void client_metrics::start()
{
    std::filesystem::create_directory(m_target_path);

    INFO << "client metrics persistence directory: " << m_target_path;
}

// ---------------------------------------------------------------------

void client_metrics::add(const uh::protocol::client_statistics::request& req)
{
    m_id_to_size.insert({std::string(req.uhv_id.begin(), req.uhv_id.end()), req.integrated_size});
}

// ---------------------------------------------------------------------

void client_metrics::serialize()
{

}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
