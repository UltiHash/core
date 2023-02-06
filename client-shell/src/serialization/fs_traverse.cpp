#include "fs_traverse.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

fs_traverse::fs_traverse(const std::vector<std::filesystem::path>& traverse_Path, common::job_queue& jq, size_t num_threads) :
        m_traversePaths(traverse_Path) , m_output_jq(jq), common::thread_manager(jq, num_threads)
{

}

// ---------------------------------------------------------------------

void fs_traverse::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&](){
            for (size_t i = 0; i < m_traversePaths.size(); ++i)
            {
                // do traversal of each path

            }
        });
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

