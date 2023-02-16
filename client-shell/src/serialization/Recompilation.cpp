#include "Recompilation.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

Recompilation::Recompilation(const uh::client::option::client_config &config,
                             std::unique_ptr<uh::protocol::client_pool>&& pool) :
                             m_config(config), m_client_pool(std::move(pool))
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
    common::job_queue<std::unique_ptr<common::f_meta_data>> qf_meta_data;
    common::job_queue<std::unique_ptr<common::f_meta_data>> qf_mdata_w_hash;

    f_upload mt_upload_class(std::move(m_client_pool), qf_meta_data,
                             qf_mdata_w_hash, m_config.m_pool_size);
    fs_traverse traverse_class(m_config.m_inputPaths, m_config.m_operatePaths, qf_meta_data);
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

