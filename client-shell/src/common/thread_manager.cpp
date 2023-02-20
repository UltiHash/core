#include "thread_manager.h"

namespace uh::client::common
{

// ---------------------------------------------------------------------------------------------------------------------

thread_manager::thread_manager(unsigned int num_threads) : m_num_threads(num_threads), m_thread_pool()
{
}

// ---------------------------------------------------------------------------------------------------------------------

thread_manager::~thread_manager ()
{
    for (auto& thread : m_thread_pool)
    {
        INFO << "Joining Thread " << thread.get_id();
        if (thread.joinable())
            thread.join();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common