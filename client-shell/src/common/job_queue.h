#ifndef COMMON_JOB_QUEUE_H
#define COMMON_JOB_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <list>
#include <atomic>
#include <optional>

namespace uh::client::common
{

// ---------------------------------------------------------------------

template <typename T>
class job_queue
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    job_queue() : m_stop_queue(false) {};
    ~job_queue() = default;

    // ------------------------------------------------- SPECIAL FUNCTIONS
    void stop()
    {
        m_stop_queue = true;
        m_cv.notify_all();
    }

    // ------------------------------------------------- GETTERS
    std::optional<T> get_job()
    {
        std::unique_lock lk(m_mutex);

        m_cv.wait(lk, [&](){ return !m_jobs.empty() || m_stop_queue; });
        if (m_stop_queue)
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
    std::list<T> m_jobs;
    std::atomic<bool> m_stop_queue;
};

// ---------------------------------------------------------------------

} // namespace uh::client::common

#endif
