#include "traverse.h"

namespace uh::client
{

// ---------------------------------------------------------------------

traverse::traverse(const std::filesystem::path& path,
                   uhv::job_queue<std::unique_ptr<uhv::meta_data>>& jq)
    : m_fs_queue(),
      m_output_jq(jq)
{
    m_fs_queue.push(path);
    run();
}

// ---------------------------------------------------------------------

void traverse::run()
{
    while(!m_fs_queue.empty())
    {
        auto path = m_fs_queue.front();
        m_fs_queue.pop();

        std::unique_ptr<uhv::meta_data> meta_data = std::make_unique<uhv::meta_data>(path);
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
