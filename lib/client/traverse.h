#ifndef CLIENT_TRAVERSE_H
#define CLIENT_TRAVERSE_H

#include <logging/logging_boost.h>
#include <uhv/job_queue.h>

#include <queue>
#include <filesystem>


namespace uh::client
{

// ---------------------------------------------------------------------

class traverse
{
public:

    traverse(const std::filesystem::path& paths,
             uhv::job_queue<std::unique_ptr<uhv::meta_data>>& jq);

    void run();

private:
    std::queue<std::filesystem::path> m_fs_queue;
    uhv::job_queue<std::unique_ptr<uhv::meta_data>>& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
