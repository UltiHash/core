#ifndef COMMON_JOB_QUEUE_H
#define COMMON_JOB_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <list>
#include <atomic>
#include <optional>
#include "f_meta_data.h"

namespace uh::client::common
{

// ---------------------------------------------------------------------------------------------------------------------

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
    void put_back_job(T&& elem)
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

        auto f_path_compare = [](const auto& a, const auto& b)
        {
            return (std::is_same<T, std::unique_ptr<f_meta_data>>::value) ? a->get_f_path() < b->get_f_path() : a < b ;
        };

        m_jobs.sort(f_path_compare);

        lk.unlock();
    }

    // -------------------------------------------------
    void print()
    {
        std::unique_lock lk(m_mutex);

        for (const auto& metadata_ptr : m_jobs)
        {
            std::cout << metadata_ptr->get_f_path() << std::endl;
//            std::cout << metadata_ptr->print_hashes();
        }

        lk.unlock();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::list<T> m_jobs;
    std::atomic<bool> m_stop_queue;
};

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common

#endif
