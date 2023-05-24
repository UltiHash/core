#ifndef CLIENT_FS_TRAVERSE_H
#define CLIENT_FS_TRAVERSE_H

#include <logging/logging_boost.h>
#include <uhv/job_queue.h>

#include <queue>
#include <filesystem>


namespace uh::client
{

// ---------------------------------------------------------------------

class f_traverse
{
public:

    f_traverse(const std::filesystem::path& traverse_Paths,
               uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq);

    void traverse();

private:
    std::queue<std::filesystem::path> m_fs_queue;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
