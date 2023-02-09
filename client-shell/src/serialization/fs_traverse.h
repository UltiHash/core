#ifndef SERIALIZATION_FS_TRAVERSE_H
#define SERIALIZATION_FS_TRAVERSE_H

#include "../common/f_meta_data.h"
#include "../common/job_queue.h"
#include <queue>
#include <filesystem>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class fs_traverse
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    fs_traverse(std::vector<std::filesystem::path> traverse_Paths, common::job_queue<std::unique_ptr<common::f_meta_data>>& jq);
    ~fs_traverse() = default;

    // ------------------------------------------------- SPECIAL FUNCTIONS
    void traverse();

    // ------------------------------------------------- GETTERS


    // ------------------------------------------------- SETTERS


private:
    std::queue<std::filesystem::path> m_fs_jq;
    common::job_queue<std::unique_ptr<common::f_meta_data>>& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
