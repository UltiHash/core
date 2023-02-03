#ifndef COMMON_THREAD_POOL_H
#define COMMON_THREAD_POOL_H

#include <thread>
#include <vector>
#include <functional>
#include "job_queue.h"
#include <logging/logging_boost.h>

namespace uh::client::common
{

// ---------------------------------------------------------------------

    template <typename T>
    class thread_manager
    {
    public:

        // ------------------------------------------------- CLASS FUNCTIONS
        thread_manager(const std::function<void(std::unique_ptr<T>)>& consume_job ,job_queue<T>& jq, size_t num_threads=1) : m_consume_job(consume_job), m_job_queue(jq), m_num_threads(num_threads)
        {
            for (size_t i = 0; i < m_num_threads; i++)
            {
                m_thread_pool.emplace_back([&](){
                    while (auto&& item = m_job_queue.get_job())
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
            m_job_queue.stop();
            for (auto& thread : m_thread_pool)
            {
                INFO << "Joining Threads";
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
        std::function<void(std::unique_ptr<T>)>& m_consume_job;
    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
