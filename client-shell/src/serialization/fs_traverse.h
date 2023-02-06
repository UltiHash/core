#ifndef SERIALIZATION_FS_TRAVERSE_H
#define SERIALIZATION_FS_TRAVERSE_H

#include "../common/thread_manager.h"
#include "common/file_meta_data.h"
#include "../common/job_queue.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class fs_traverse : public common::thread_manager
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    explicit fs_traverse(const std::vector<std::filesystem::path>& traverse_Path, common::job_queue& jq, size_t num_threads=1);
    ~fs_traverse() override = default;
    void spawn_threads() override;

    // ------------------------------------------------- GETTERS


    // ------------------------------------------------- SETTERS


private:
    std::vector<std::filesystem::path> m_traversePaths;
    common::job_queue& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
