#ifndef SERVER_DATABASE_PERSISTENCE_MOD_H
#define SERVER_DATABASE_PERSISTENCE_MOD_H

#include <persistence/storage/job_queue_persistence.h>
#include <persistence/options.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

    class mod
    {
    public:
        explicit mod(const persistence_config& config);

        ~mod() = default;

        void start();

        job_queue_persistence& jobQ_persistence();

    private:
        std::unique_ptr<job_queue_persistence> m_jq_persistence;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence

#endif
