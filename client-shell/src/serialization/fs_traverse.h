#ifndef SERIALIZATION_FS_TRAVERSE_H
#define SERIALIZATION_FS_TRAVERSE_H

#include <queue>
#include <filesystem>
#include <logging/logging_boost.h>
#include "../common/f_meta_data.h"
#include "../common/job_queue.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class fs_traverse
{
public:

    fs_traverse(std::vector<std::filesystem::path>,
                std::vector<std::filesystem::path>,
                common::job_queue<std::unique_ptr<common::f_meta_data>>&);
    ~fs_traverse() = default;

    void traverse();

private:
    std::queue<std::filesystem::path> m_fs_jq;
    std::vector<std::filesystem::path> m_operate_paths;
    common::job_queue<std::unique_ptr<common::f_meta_data>>& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
