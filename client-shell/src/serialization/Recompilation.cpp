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

    q_f_mdata_w_hash.sort();

    f_serialization serializer(m_config.m_outputPath, q_f_mdata_w_hash);
    serializer.serialize(m_config.m_inputPaths);

}

// ---------------------------------------------------------------------------------------------------------------------

void Recompilation::retrieve()
{
    std::filesystem::create_directories(m_config.m_outputPath);
    common::job_queue<std::unique_ptr<common::f_meta_data>> q_f_meta_data;

//    {
//        f_download download_class(m_client_pool, q_f_meta_data, m_config.m_outputPath,m_config.m_thread_size);
//        download_class.spawn_threads();
//    }

    for (const auto& uhv_file_path: m_config.m_inputPaths)
    {
        f_serialization deserializer(uhv_file_path, q_f_meta_data);
        deserializer.deserialize();
    }

}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization

