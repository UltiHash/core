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

void f_download::download_files(std::unique_ptr<common::f_meta_data>& f_meta_data,
                            protocol::client_pool::handle& client_handle)
{

}

// ---------------------------------------------------------------------------------------------------------------------

f_download::~f_download()
{
    m_input_jq.stop();
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
