#ifndef SERVER_DATABASE_PERSISTENCE_JOB_QUEUE_PERSISTENCE_H
#define SERVER_DATABASE_PERSISTENCE_JOB_QUEUE_PERSISTENCE_H

#include <filesystem>
#include <options/options.h>
#include <persistence/options.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

    class job_queue_persistence
    {
    public:
        explicit job_queue_persistence(const persistence_config& config);

        void start();
        void add();
        void flush();

    private:
        void retrieve();
        std::filesystem::path m_target_path;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn::persistence

#endif
