#include "entry_job.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

    entry_job::entry_job(int id, uh::cluster::cluster_ranks cluster_plan, uh::rest::rest_server_config&& rest_config) :
            m_cluster_plan (std::move (cluster_plan)),
            m_id (id),
            m_job_name ("entry_" + std::to_string (id))
    {
        m_rest_server = std::make_unique<uh::rest::rest_server>(std::move(rest_config));
    }

//------------------------------------------------------------------------------

    void
    entry_job::run()
    {
        m_rest_server->run();
    }

//------------------------------------------------------------------------------

} // namespace uh::cluster