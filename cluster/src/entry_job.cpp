#include "entry_job.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

entry_job::entry_job(int id, entry_node_config config, cluster_map&& cmap) :
        m_cluster_map (std::move (cmap)),
        m_id (id),
        m_job_name ("entry_" + std::to_string (id)),
        m_internal_server (config.internal_server_conf),
        m_rest_server (config.rest_server_conf)
{
}

//------------------------------------------------------------------------------

void
entry_job::run()
{
    m_rest_server.run();
}

//------------------------------------------------------------------------------

} // namespace uh::cluster