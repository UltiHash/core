#include "f_upload.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_upload::f_upload(std::unique_ptr<protocol::client_pool>&& cl_pool, common::job_queue<std::unique_ptr<common::f_meta_data>>& in_jq, common::job_queue<std::unique_ptr<common::f_meta_data>>& out_jq, size_t num_threads) :
    m_client_pool(std::move(cl_pool)), m_input_jq(in_jq), m_output_jq(out_jq), common::thread_manager(in_jq, num_threads)
{

}

// ---------------------------------------------------------------------

f_upload::~f_upload()
{
    m_input_jq.stop();
}

// ---------------------------------------------------------------------

void f_upload::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&](){
            while (auto&& item = m_input_jq.get_job())
            {
                if (item == std::nullopt)
                    break;
//                else
//                    fupload(item);
            }
        });
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
