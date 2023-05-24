#include "traverse.h"

namespace uh::client
{

// ---------------------------------------------------------------------

f_traverse::f_traverse(std::vector<std::filesystem::path> traverse_Paths,
                       uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq)
    : m_fs_queue(),
      m_output_jq(jq)
{
    for (auto& item : traverse_Paths)
        m_fs_queue.push(std::move(item));
    traverse();
}

// ---------------------------------------------------------------------

void f_traverse::traverse()
{

    while(!m_fs_queue.empty())
    {
        auto path = m_fs_queue.front();
        m_fs_queue.pop();

        std::unique_ptr<uhv::f_meta_data> meta_data = std::make_unique<uhv::f_meta_data>(path);
        m_output_jq.append_job(std::move(meta_data));

        if (is_directory(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                m_fs_queue.push(entry.path());
            }
        }
    }

}

// ---------------------------------------------------------------------

} // namespace uh::client
