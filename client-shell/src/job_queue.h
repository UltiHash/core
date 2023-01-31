#ifndef COMMON_JOB_QUEUE_H
#define COMMON_JOB_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <list>

namespace uh::client
{

// ---------------------------------------------------------------------

    template <typename T>
    class job_queue
    {
    public:
        // GETTERS
        T&& get();

        // SETTERS
        void put_back(T&& elem);

    protected:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::list<T> m_list;
    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
