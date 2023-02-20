#include "Recompilation.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------------------------------

void Recompilation::integrate()
{
    common::job_queue<std::unique_ptr<common::f_meta_data>> q_f_meta_data;
    common::job_queue<std::unique_ptr<common::f_meta_data>> q_f_mdata_w_hash;

    {
        f_upload upload_class(m_client_pool, q_f_meta_data,
                              q_f_mdata_w_hash, m_config.m_thread_size);
        upload_class.spawn_threads();
        f_traverse traverse_class(m_config.m_inputPaths, m_config.m_operatePaths, q_f_meta_data);
    }

    std::cout << "\n\nBefore sorting: \n";
    q_f_mdata_w_hash.print();
    q_f_mdata_w_hash.sort();
    std::cout << "\n\nAfter sorting: \n";
    q_f_mdata_w_hash.print();

    // serialize everything in a recompilation file

}

// ---------------------------------------------------------------------------------------------------------------------

void Recompilation::retrieve()
{
    //TODO:
}

// ---------------------------------------------------------------------------------------------------------------------

void Recompilation::list()
{
    //TODO:
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization

