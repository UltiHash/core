#include "f_upload.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_upload::f_upload(const std::vector<std::filesystem::path>& traverse_Path, common::job_queue& jq, size_t num_threads=1);

// ---------------------------------------------------------------------

f_upload::~f_upload()
{
    m_job_queue.stop();
}

// ---------------------------------------------------------------------

void f_upload::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&](){
            while (auto&& item = m_job_queue.get_job())
            {
                if (item == std::nullopt)
                    break;
                else
                    fupload(item);
            }
        });
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
