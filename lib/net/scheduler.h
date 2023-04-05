#ifndef NET_SCHEDULER_H
#define NET_SCHEDULER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>


namespace uh::net
{

// ---------------------------------------------------------------------

class scheduler
{
public:
    explicit scheduler(std::size_t threads);
    ~scheduler();

    void spawn(const std::function<void()>& f);
    void worker();
    void stop();

    [[nodiscard]] bool is_busy() const;

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::list<std::function<void()>> m_jobs;
    std::list<std::thread> m_threads;

    std::atomic<bool> m_running;
    std::atomic<std::size_t> m_threads_used;

    std::size_t m_threads_limit;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
