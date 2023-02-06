#include "fs_traverse.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

fs_traverse::fs_traverse(std::vector<std::filesystem::path> traverse_Paths, common::job_queue<std::unique_ptr<common::f_meta_data>>& jq, size_t num_threads) :
        m_output_jq(jq), common::thread_manager(num_threads)
{
    // no threads spawned so it is slopw since mutex lock and unlock all the time
    for (auto& path : traverse_Paths)
    {
        m_fs_jq.put_back_job(std::move(path));
    }
}

// ---------------------------------------------------------------------

fs_traverse::~fs_traverse()
{
    m_fs_jq.stop();
}

// ---------------------------------------------------------------------

void fs_traverse::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&](){
        });
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

