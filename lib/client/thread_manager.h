#ifndef CLIENT_THREAD_MANAGER_H
#define CLIENT_THREAD_MANAGER_H

#include <thread>
#include <vector>
#include <logging/logging_boost.h>

namespace uh::client
{

// ---------------------------------------------------------------------

class thread_manager
{
public:

    explicit thread_manager(unsigned int);
    virtual ~thread_manager();
    
    virtual void spawn_threads()=0;

protected:
    unsigned int m_num_threads;
    std::vector<std::thread> m_thread_pool;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
