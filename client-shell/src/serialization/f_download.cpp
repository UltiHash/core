#include "f_download.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

f_download::f_download(std::unique_ptr<protocol::client_pool>& cl_pool,
                       common::job_queue<std::unique_ptr<common::f_meta_data>>& jq,
                       std::filesystem::path dest_path,
                       unsigned int num_threads) :
                       m_client_pool(cl_pool),
                       m_input_jq(jq),
                       m_dest_path(std::move(dest_path)),
                       common::thread_manager(num_threads)

{
}

// ---------------------------------------------------------------------------------------------------------------------

f_download::~f_download()
{
    m_input_jq.stop();
}

// ---------------------------------------------------------------------------------------------------------------------

void f_download::download_files(std::unique_ptr<common::f_meta_data>& f_meta_data,
                            protocol::client_pool::handle& client_handle)
{
    std::filesystem::path new_f_path = m_dest_path / f_meta_data->get_f_path();
    if (S_ISDIR(f_meta_data->get_f_stat().st_mode))
    {
        if (!std::filesystem::exists(new_f_path))
        {
            std::filesystem::create_directories(new_f_path);
        }
    }
    else if (S_ISREG(f_meta_data->get_f_stat().st_mode))
    {
        std::ofstream new_file(new_f_path,
                                 std::ios::app | std::ios::binary);

        if (!new_file.is_open())
            throw std::runtime_error("Error: Could not open file "
                                     + f_meta_data->get_f_path().string() + "\n");

        std::vector<char> buffer(64);

        for (size_t i = 0; i < f_meta_data->get_f_hashes().size(); i += 64)
        {
            std::copy(f_meta_data->get_f_hashes().begin() + i,
                      f_meta_data->get_f_hashes().begin() + i + 64, buffer.begin());

            auto data = client_handle->read_chunk(buffer);
            new_file.write(&data[0], data.size());
        }

        new_file.flush();
        new_file.close();
    }
    else
    {
        throw std::runtime_error("Unknown file type encountered." + f_meta_data->get_f_path().string());
    }

    std::filesystem::permissions(new_f_path,
                                 std::filesystem::perms(f_meta_data->get_f_stat().st_mode));

    struct timeval times[2];
    times[0].tv_sec = f_meta_data->get_f_stat().st_atim.tv_sec;
    times[0].tv_usec = f_meta_data->get_f_stat().st_atim.tv_nsec;
    times[1].tv_sec = f_meta_data->get_f_stat().st_mtim.tv_sec;
    times[1].tv_usec = f_meta_data->get_f_stat().st_mtim.tv_nsec;
    if (utimes(new_f_path.c_str(), times) != 0)
    {
        throw std::runtime_error("Cannot update utime/atime: " + new_f_path.string());
    }

    if (chown(new_f_path.c_str(), f_meta_data->get_f_stat().st_uid, f_meta_data->get_f_stat().st_gid) != 0)
    {
        throw std::runtime_error("Failed to update uid and gid of " + new_f_path.string());
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void f_download::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&]()
            {
               protocol::client_pool::handle&& client_connection_handle = m_client_pool->get();

               while (auto&& item = m_input_jq.get_job())
               {
                   if (item == std::nullopt)
                       break;
                   else
                       download_files(item.value(),
                                    client_connection_handle);

               }

            });
    }
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization
