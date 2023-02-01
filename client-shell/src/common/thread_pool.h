#ifndef COMMON_THREAD_POOL_H
#define COMMON_THREAD_POOL_H

#include <thread>
#include <vector>
#include "job_queue.h"

namespace uh::client
{

// ---------------------------------------------------------------------

    template <typename T>
    class thread_manager
    {
    public:

        // ------------------------------------------------- CLASS FUNCTIONS
        thread_manager(int num_threads, job_queue<T>& jq) : m_job_queue(jq)
        {

        }

        ~thread_manager() {
            for (auto& worker : m_worker_threads)
            {
                worker.join();
            }
        }

        // ------------------------------------------------- GETTERS


        // ------------------------------------------------- SETTERS


    private:
        job_queue<T>& m_job_queue;
        std::vector<std::thread> m_worker_threads;

    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
