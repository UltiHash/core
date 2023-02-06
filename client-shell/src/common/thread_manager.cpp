#include "thread_manager.h"

namespace uh::client::common
{

// ---------------------------------------------------------------------

thread_manager::thread_manager(job_queue& jq, size_t num_threads) : m_job_queue(jq), m_num_threads(num_threads), m_thread_pool()
{
}

// ---------------------------------------------------------------------

thread_manager::~thread_manager ()
{
    for (auto& thread : m_thread_pool)
    {
        INFO << "Joining Threads";
        if (thread.joinable())
        thread.join();
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::common