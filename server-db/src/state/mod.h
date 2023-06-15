#ifndef SERVER_DATABASE_STATE_MOD_H
#define SERVER_DATABASE_STATE_MOD_H

#include <state/scheduled_compressions_state.h>
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

        scheduled_compressions_state& scheduled_compressions();

    private:
        std::unique_ptr<scheduled_compressions_state> m_scheduled_compressions_state;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn:state

#endif
