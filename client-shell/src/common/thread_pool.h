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
            for (size_t i = 0; i < m_num_threads; i++)
            {
                m_thread_pool.emplace_back([&](){
                    while (auto item = m_job_queue.get_job())
                    {
                        if (item == std::nullopt)
                            break;
                        else
                            m_consume_job(item);
                    }
                });
            }
        }

        ~thread_manager()
        {
            for (auto& thread : m_thread_pool)
            {
                m_job_queue.stop();
                if (thread.joinable())
                    thread.join();
            }
        }

        // ------------------------------------------------- GETTERS


        // ------------------------------------------------- SETTERS


    private:
        size_t m_num_threads;
        job_queue<std::unique_ptr<T>>& m_job_queue;
        std::vector<std::thread> m_thread_pool;
        std::function<void(T)>& m_consume_job;
    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
