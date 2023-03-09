#include "scheduler.h"

#include <logging/logging_boost.h>

#include <exception>


namespace uh::net
{

// ---------------------------------------------------------------------

scheduler::scheduler(std::size_t threads)
{
    while (threads--)
    {
        m_threads.emplace_back([this](){ this->worker(); });
    }
}

// ---------------------------------------------------------------------

scheduler::~scheduler()
{
    stop();
}

// ---------------------------------------------------------------------

void scheduler::spawn(const std::function<void()>& f)
{
    {
        std::lock_guard lk(m_mutex);

        m_jobs.push_back(f);
    }

    m_cv.notify_one();
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

        m_cv.wait(lk, [&](){ return !m_jobs.empty(); });

        auto job = m_jobs.front();
        m_jobs.pop_front();
        lk.unlock();

        try
        {
            job();
        }
        catch (const std::exception& e)
        {
            ERROR << "job failed: " << e.what();
        }
        catch (...)
        {
            ERROR << "job failed with unknown exception";
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
