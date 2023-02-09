#include "fs_traverse.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

fs_traverse::fs_traverse(std::vector<std::filesystem::path> traverse_Paths, common::job_queue<std::unique_ptr<common::f_meta_data>>& jq) :
    m_fs_jq(), m_output_jq(jq)
{
    for (const auto& item : traverse_Paths)
        m_fs_jq.push(std::move(item));
}

// ---------------------------------------------------------------------

void fs_traverse::traverse()
{
    while(!m_fs_jq.empty())
    {
        auto path = m_fs_jq.front();
        m_fs_jq.pop();
        // create a f_meta_data out of path
        // push it to the output job queue
        // traverse more
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                m_fs_jq.push(entry.path());
            }
        }
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

