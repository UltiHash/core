#ifndef COMMON_JOB_QUEUE_H
#define COMMON_JOB_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <list>
#include <atomic>
#include <optional>
#include "file_meta_data.h"

namespace uh::client::common
{

// ---------------------------------------------------------------------

class job_queue
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    job_queue();
    ~job_queue() = default;

    // ------------------------------------------------- SPECIAL FUNCTIONS
    void stop();

    // ------------------------------------------------- GETTERS
    std::optional<std::unique_ptr<file_meta_data>> get_job();

    // ------------------------------------------------- SETTERS
    void put_back_job(std::unique_ptr<file_meta_data>&& elem);

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::list<std::unique_ptr<file_meta_data>> m_jobs;
    std::atomic<bool> m_stop_queue;
};

// ---------------------------------------------------------------------

} // namespace uh::client::common

#endif
