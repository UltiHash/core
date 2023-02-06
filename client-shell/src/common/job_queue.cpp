#include "job_queue.h"

namespace uh::client::common
{

// ---------------------------------------------------------------------

job_queue::job_queue() : m_stop_queue(false)
{
}

// ---------------------------------------------------------------------

void job_queue::stop()
{
    m_stop_queue = true;
    m_cv.notify_all();
}

// ---------------------------------------------------------------------

std::optional<std::unique_ptr<f_meta_data>> job_queue::get_job()
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

// ---------------------------------------------------------------------

void job_queue::put_back_job(std::unique_ptr<f_meta_data>&& elem)
{
    std::unique_lock lk(m_mutex);

    m_jobs.push_back(std::move(elem));

    lk.unlock();
    m_cv.notify_one();
}

// ---------------------------------------------------------------------

} // namespace uh::client::common
