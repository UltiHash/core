#ifndef COMMON_THREAD_POOL_H
#define COMMON_THREAD_POOL_H

#include <thread>
#include <vector>
#include <logging/logging_boost.h>

namespace uh::client::common
{

// ---------------------------------------------------------------------
class thread_manager
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    explicit thread_manager(size_t);
    virtual ~thread_manager();

    // ------------------------------------------------- SPECIAL FUNCTIONS
    virtual void spawn_threads()=0;

protected:
    size_t m_num_threads;
    std::vector<std::thread> m_thread_pool;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
