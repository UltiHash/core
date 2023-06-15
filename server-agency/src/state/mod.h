#ifndef SERVER_AGENCY_PERSISTENCE_MDD_H
#define SERVER_AGENCY_PERSISTENCE_MDD_H

#include <memory>
#include <state/options.h>
#include <state/client_metrics_state.h>

namespace uh::an::state
{

// ---------------------------------------------------------------------

    class mod
    {
    public:
        explicit mod(const storage_config& config);

        ~mod() = default;

        void start();
        void stop();

        client_metrics_state& client_metrics();

    private:
        std::unique_ptr<client_metrics_state> m_client_metrics_state;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
