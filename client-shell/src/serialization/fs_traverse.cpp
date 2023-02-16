#include "fs_traverse.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

fs_traverse::fs_traverse(std::vector<std::filesystem::path> traverse_Paths,
                         std::vector<std::filesystem::path> operate_Paths,
                         common::job_queue<std::unique_ptr<common::f_meta_data>>& jq) :
                         m_fs_jq(), m_operate_paths(std::move(operate_Paths)) , m_output_jq(jq)
{
    for (auto& item : traverse_Paths)
        m_fs_jq.push(std::move(item));
    traverse();
}

// ---------------------------------------------------------------------------------------------------------------------

void fs_traverse::traverse()
{
    while(!m_fs_jq.empty())
    {
        auto path = m_fs_jq.front();
        m_fs_jq.pop();

        std::unique_ptr<common::f_meta_data> meta_data = std::make_unique<common::f_meta_data>(path);
        std::cout << meta_data->get_f_path() << "\n";
        m_output_jq.put_back_job(std::move(meta_data));

        if (is_directory(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                m_fs_jq.push(entry.path());
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization
