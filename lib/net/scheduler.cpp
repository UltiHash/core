#include "scheduler.h"


namespace uh::net
{

// ---------------------------------------------------------------------

scheduler::scheduler(std::size_t threads)
{
    while (threads--)
    {
        m_threads.push_back(std::thread([this](){ this->worker(); }));
    }
}

// ---------------------------------------------------------------------

void scheduler::spawn(std::function<void()> f)
{
    {
        std::unique_lock lk(m_mutex);

        m_jobs.push_back(f);
    }

    m_cv.notify_all();
}

// ---------------------------------------------------------------------

void scheduler::worker()
{
    m_running = true;
    while (m_running)
    {
        std::unique_lock lk(m_mutex);
        if (!m_running)
        {
            break;
        }

        if (m_jobs.empty())
        {
            m_cv.wait(lk);
        }
        else
        {
            auto job = m_jobs.front();
            m_jobs.pop_front();
            lk.unlock();
            job();
        }
    }
}

// ---------------------------------------------------------------------

void scheduler::stop()
{
    m_running = false;
    m_cv.notify_all();

    for (auto& th : m_threads)
    {
        th.join();
    }
}

// ---------------------------------------------------------------------

} // namespace uh::net
