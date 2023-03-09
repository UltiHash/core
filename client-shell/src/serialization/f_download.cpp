#include "f_download.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

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

// ---------------------------------------------------------------------

f_download::~f_download()
{
    m_input_jq.stop();
    for (auto& thread : std::ranges::reverse_view(m_thread_pool))
    {
        INFO << "Joining Thread ";
        if (thread.joinable())
        {
            thread.join();
            m_thread_pool.pop_back();
        }
    }
}

// ---------------------------------------------------------------------

void f_download::download_files(std::unique_ptr<common::f_meta_data>& f_meta_data,
                            protocol::client_pool::handle& client_handle)
{
    if (f_meta_data->f_type() == uh::client::common::uh_file_type::regular)
    {
        std::ofstream new_file(f_meta_data->f_path(),
                                 std::ios::app | std::ios::binary);

        if (!new_file.is_open())
            throw std::runtime_error("Error: Could not open file "
                                     + f_meta_data->f_path().string() + "\n");

        std::vector<char> buffer(64);

        for (auto i = 0; i < f_meta_data->f_hashes().size(); i += 64)
        {
            std::copy(f_meta_data->f_hashes().begin() + i,
                      f_meta_data->f_hashes().begin() + i + 64, buffer.begin());

            new_file << client_handle->read_block(buffer);
        }

        new_file.flush();
        new_file.close();
    }
    else if (f_meta_data->f_type() == uh::client::common::uh_file_type::none)
    {
        throw std::runtime_error("Unknown file type encountered: " + f_meta_data->f_path().string());
    }

    std::filesystem::permissions(f_meta_data->f_path(),
                                 static_cast<std::filesystem::perms>(f_meta_data->f_permissions()),
                                 std::filesystem::perm_options::replace);

}

// ---------------------------------------------------------------------

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

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
