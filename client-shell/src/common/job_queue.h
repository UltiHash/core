#ifndef COMMON_JOB_QUEUE_H
#define COMMON_JOB_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <list>
#include <atomic>

namespace uh::client
{

// ---------------------------------------------------------------------

template <typename T>
class job_queue
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    job_queue() : m_stop(false)
    {
    }

    // ------------------------------------------------- LOGIC FUNCTIONS
    void stop()
    {
        stop_queue = true;
        m_cv.notify_all();
    }

    // ------------------------------------------------- GETTERS
    std::optional<std::unique_ptr<T>&&> get_job()
    {
        std::unique_lock lk(m_mutex);

        m_cv.wait(lk, [&](){ return !m_jobs.empty() || stop_queue; });
        if (stop_queue)
        {
            return std::nullopt;
        }

        auto job = std::move(m_jobs.front());
        m_jobs.pop_front();

        return (std::move(job));
    }

    // ------------------------------------------------- SETTERS
    void put_back_job(T&& elem)
    {
        std::unique_lock lk(m_mutex);

        m_jobs.push_back(std::move(elem));

        lk.unlock();
        m_cv.notify_one();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::list<std::unique_ptr<T>> m_jobs;
    std::atomic<bool> m_stop;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
