#ifndef SERVER_AGENCY_SCHEDULER_H
#define SERVER_AGENCY_SCHEDULER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>


namespace uh::an
{

// ---------------------------------------------------------------------

class scheduler
{
public:
    scheduler(std::size_t threads);

    void spawn(std::function<void()> f);

    void worker();

    void stop();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::list<std::function<void()>> m_jobs;

    std::list<std::thread> m_threads;
    std::atomic<bool> m_running;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
