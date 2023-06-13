#ifndef UHV_JOB_QUEUE_H
#define UHV_JOB_QUEUE_H

#include <uhv/meta_data.h>

#include <condition_variable>
#include <mutex>
#include <list>
#include <atomic>
#include <optional>


namespace uh::uhv
{

// ---------------------------------------------------------------------

template <typename T>
class job_queue
{
public:

    // -------------------------------------------------
    job_queue() : m_stop_queue(false) {};
    ~job_queue() = default;

    // -------------------------------------------------
    void stop()
    {
        m_stop_queue = true;
        m_cv.notify_all();
    }

    // -------------------------------------------------
    std::optional<T> get_job()
    {
        std::unique_lock lk(m_mutex);

        m_cv.wait(lk, [&](){ return !m_jobs.empty() || m_stop_queue; });
        if (m_stop_queue && m_jobs.empty())
        {
            return std::nullopt;
        }

        auto job = std::move(m_jobs.front());
        m_jobs.pop_front();

        return (std::move(job));

    }

    // -------------------------------------------------
    void append_job(T&& elem)
    {
        std::unique_lock lk(m_mutex);

        m_jobs.push_back(std::move(elem));

        lk.unlock();
        m_cv.notify_one();
    }

    // -------------------------------------------------
    void sort()
    {
        std::unique_lock lk(m_mutex);

        auto compare = [](const auto& a, const auto& b)
        {
            return (std::is_same<T, std::unique_ptr<meta_data>>::value) ?
                        a->path() < b->path() : a < b ;
        };

        m_jobs.sort(compare);

        lk.unlock();
    }

    // -------------------------------------------------
    unsigned long size()
    {
        std::lock_guard lk(m_mutex);

        return m_jobs.size();
    }

    // -------------------------------------------------
    bool empty()
    {
        std::unique_lock lk(m_mutex);

        return m_jobs.empty();
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

