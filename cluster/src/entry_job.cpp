#include "entry_job.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

    entry_job::entry_job(int id, server_config rest_config, cluster_map&& cmap) :
            m_cluster_map (std::move (cmap)),
            m_id (id),
            m_job_name ("entry_" + std::to_string (id)),
            m_rest_server (rest_config)
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