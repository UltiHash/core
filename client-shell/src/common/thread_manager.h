#ifndef COMMON_THREAD_POOL_H
#define COMMON_THREAD_POOL_H

#include <thread>
#include <vector>
#include <functional>
#include "job_queue.h"
#include "file_meta_data.h"
#include <logging/logging_boost.h>

namespace uh::client::common
{

// ---------------------------------------------------------------------
class thread_basics
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    thread_basics(job_queue& jq, size_t num_threads);
    virtual ~thread_basics();

    // ------------------------------------------------- SPECIAL FUNCTIONS
    virtual void spawn_threads();

protected:
    size_t m_num_threads;
    job_queue& m_job_queue;
    std::vector<std::thread> m_thread_pool;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
