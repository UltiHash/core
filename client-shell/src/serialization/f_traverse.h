#ifndef SERIALIZATION_FS_TRAVERSE_H
#define SERIALIZATION_FS_TRAVERSE_H

#include <logging/logging_boost.h>
#include <uhv/job_queue.h>

#include <queue>
#include <filesystem>


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_traverse
{
public:

    f_traverse(std::vector<std::filesystem::path>,
               std::vector<std::filesystem::path>,
               uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>&);
    ~f_traverse() = default;

    void traverse();

private:
    std::queue<std::filesystem::path> m_fs_queue;
    std::vector<std::filesystem::path> m_operate_paths;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
