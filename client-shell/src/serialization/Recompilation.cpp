#include "Recompilation.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

Recompilation::Recompilation(const uh::client::option::client_config &config,
                             std::unique_ptr<uh::protocol::client_pool>&& pool) :
                             m_config(config), m_client_pool(std::move(pool))
{

    if (m_config.m_option == co::options_chosen::integrate) {
        try
        {
            integrate();
        }
        catch (const std::exception &exc)
        {
            std::filesystem::remove(m_config.m_outputPath);
            throw exc;
        }
    }
    else if (m_config.m_option == co::options_chosen::retrieve)
        retrieve();


}

// ---------------------------------------------------------------------

void Recompilation::integrate()
{
    auto time_start = std::chrono::system_clock::now();

    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>> q_f_meta_data;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>> q_f_mdata_w_hash;

    {
        f_upload upload_class(m_client_pool, q_f_meta_data,
                              q_f_mdata_w_hash, m_config.m_worker_count);
        upload_class.spawn_threads();
        f_traverse traverse_class(m_config.m_inputPaths, m_config.m_operatePaths, q_f_meta_data);
    }

    q_f_mdata_w_hash.sort();

    f_serialization serializer(m_config.m_outputPath, q_f_mdata_w_hash);
    uint64_t size = serializer.serialize(m_config.m_inputPaths);

    auto time_end = std::chrono::system_clock::now();

    auto time_diff = std::chrono::duration<double>(time_end - time_start);
    double seconds = time_diff.count();
    double mbytes = static_cast<double>(size) / (1024*1024);

    std::cout << "encoding speed: " << (mbytes / seconds) << " Mb/s" << std::endl;

}

// ---------------------------------------------------------------------

void Recompilation::retrieve()
{
    uint64_t size = 0;
    auto time_start = std::chrono::system_clock::now();

    std::filesystem::create_directories(m_config.m_outputPath);
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>> q_f_meta_data;

    {
        f_download download_class(m_client_pool, q_f_meta_data, m_config.m_outputPath,
                                  m_config.m_worker_count);
        download_class.spawn_threads();

        f_serialization deserializer(m_config.m_inputPaths[0], q_f_meta_data);
        size = deserializer.deserialize(m_config.m_outputPath);
    }

    auto time_end = std::chrono::system_clock::now();

    auto time_diff = std::chrono::duration<double>(time_end - time_start);
    double seconds = time_diff.count();
    double mbytes = static_cast<double>(size) / (1024*1024);

    std::cout << "decoding speed: " << (mbytes / seconds) << " Mb/s" << std::endl;

}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

