#ifndef UTIL_JOB_QUEUE_H
#define UTIL_JOB_QUEUE_H

#include <condition_variable>
#include <concepts>
#include <future>
#include <list>
#include <queue>


namespace uh::util
{

// ---------------------------------------------------------------------

template <typename result, typename ... args>
class job_queue
{
public:
    /**
     * Construct a job_queue with the given number of worker threads.
     */
    job_queue(std::size_t threads, std::function<result(args...)> h);

    /**
     * Stop the queue and join all threads.
     */
    ~job_queue();

    /**
     * Add a job to the queue and receive a std::future<result>.
     */
    std::future<result> push(args...);

    /**
     * Stop the queue and join all worker threads. The queue will not
     * process any more jobs afterwards.
     */
    void stop();

    /**
     * Wait for all jobs to be finished. Returns immediately if queue is empty.
     */
    void wait();

private:
    struct entry
    {
        entry(args... a) : job(std::make_tuple(a...)) { }

        std::promise<result> promise;
        std::tuple<args...> job;
    };

    void worker();

    std::function<result(args...)> m_handler;
    std::queue<entry> m_jobs;
    std::mutex m_jobs_mutex;
    std::condition_variable m_jobs_condition;
    std::condition_variable m_empty_condition;
    std::atomic<bool> m_running;
    std::list<std::thread> m_threads;
};

// ---------------------------------------------------------------------

template <typename result, typename ... args>
job_queue<result, args...>::job_queue(std::size_t threads, std::function<result(args...)> h)
    : m_handler(h),
      m_running(true)
{
    for (std::size_t t = 0u; t < threads; ++t)
    {
        m_threads.push_back(std::thread(&job_queue<result, args...>::worker, this));
    }
}

// ---------------------------------------------------------------------

template <typename result, typename ... args>
job_queue<result, args...>::~job_queue()
{
    try
    {
        stop();
    }
    catch (...)
    {
    }
}

// ---------------------------------------------------------------------

template <typename result, typename ... args>
std::future<result> job_queue<result, args...>::push(args... a)
{
    std::unique_lock<std::mutex> lock(m_jobs_mutex);

    entry& e = m_jobs.emplace(std::forward<args>(a)...);

    m_jobs_condition.notify_one();

    return e.promise.get_future();
}

// ---------------------------------------------------------------------

template <typename result, typename ... args>
void job_queue<result, args...>::worker()
{
    while (m_running)
    {
        std::unique_lock<std::mutex> lock(m_jobs_mutex);

        if (m_jobs.empty())
        {
            m_empty_condition.notify_all();
        }

        m_jobs_condition.wait(lock, [this](){ return !(m_jobs.empty() && m_running); });

        if (!m_running)
        {
            m_empty_condition.notify_all();
            break;
        }

        auto entry = std::move(m_jobs.front());
        m_jobs.pop();

        lock.unlock();

        try
        {
            auto applied = std::apply(m_handler, entry.job);
            if (std::is_same<void, result>::value)
            {
                entry.promise.set_value();
            }
            else
            {
                entry.promise.set_value(std::move(applied));
            }
        }
        catch (...)
        {
            entry.promise.set_exception(std::current_exception());
        }
    }
}

// ---------------------------------------------------------------------

template <typename result, typename ... args>
void job_queue<result, args...>::stop()
{
    {
        std::unique_lock<std::mutex> lock(m_jobs_mutex);
        m_running = false;
        m_jobs_condition.notify_all();
    }

    for (auto& thread : m_threads)
    {
        thread.join();
    }

    m_threads.clear();
}

// ---------------------------------------------------------------------

template <typename result, typename ... args>
void job_queue<result, args...>::wait()
{
    std::unique_lock<std::mutex> lock(m_jobs_mutex);
    m_empty_condition.wait(lock, [this](){ return m_jobs.empty(); });
}

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
