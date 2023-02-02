#ifndef COMMON_THREAD_POOL_H
#define COMMON_THREAD_POOL_H

#include <thread>
#include <vector>
#include <functional>
#include "job_queue.h"

namespace uh::client
{

// ---------------------------------------------------------------------

    template <typename T>
    class thread_manager
    {
    public:

        // ------------------------------------------------- CLASS FUNCTIONS
        thread_manager(const std::function<void(T)>& consume_job ,job_queue<T>& jq, size_t num_threads=1) : m_job_queue(jq), m_num_threads(num_threads), m_consume_job(consume_job)
        {
            for (size_t i = 0; i < m_num)
            {

            }
        }

        ~thread_manager()
        {
            for (auto& worker : m_worker_threads)
            {
                worker.join();
            }
        }

        // ------------------------------------------------- GETTERS


        // ------------------------------------------------- SETTERS


    private:
        job_queue<T>& m_job_queue;
        size_t m_num_threads;
        std::vector<std::thread> m_worker_threads;
        std::function<void(T)>& m_consume_job;
    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
