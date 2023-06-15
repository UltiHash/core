#ifndef SERVER_DATABASE_PERSISTENCE_MOD_H
#define SERVER_DATABASE_PERSISTENCE_MOD_H

#include <state/scheduled_compressions.h>
#include <storage/options.h>

namespace uh::dbn::state
{

// ---------------------------------------------------------------------

    class mod
    {
    public:
        explicit mod(const uh::dbn::storage::storage_config& config);

        ~mod() = default;

        void start();

        scheduled_compressions& scheduled_compressions_state();

    private:
        std::unique_ptr<scheduled_compressions> m_scheduling;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence

#endif
