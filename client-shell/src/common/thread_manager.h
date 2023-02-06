#ifndef COMMON_THREAD_POOL_H
#define COMMON_THREAD_POOL_H

#include <thread>
#include <vector>
#include <functional>
#include "job_queue.h"
#include "f_meta_data.h"
#include <logging/logging_boost.h>

namespace uh::client::common
{

// ---------------------------------------------------------------------
class thread_manager
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    explicit thread_manager(size_t num_threads);
    virtual ~thread_manager();

    // ------------------------------------------------- SPECIAL FUNCTIONS
    virtual void spawn_threads();

protected:
    size_t m_num_threads;
    std::vector<std::thread> m_thread_pool;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
