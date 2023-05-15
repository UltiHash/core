#ifndef SERVER_DATABASE_PERSISTENCE_MOD_H
#define SERVER_DATABASE_PERSISTENCE_MOD_H

#include <persistence/storage/scheduled_compressions_persistence.h>
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

        scheduled_compressions_persistence& scheduled_persistence();

    private:
        std::unique_ptr<scheduled_compressions_persistence> m_scheduling_persistence;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence

#endif
