#include "thread_manager.h"

namespace uh::client
{

// ---------------------------------------------------------------------

thread_manager::thread_manager(unsigned int num_threads) : m_num_threads(num_threads), m_thread_pool()
{
}

// ---------------------------------------------------------------------

thread_manager::~thread_manager () = default;

// ---------------------------------------------------------------------

} // namespace uh::client
