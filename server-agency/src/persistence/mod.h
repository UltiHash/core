#ifndef SERVER_AGENCY_PERSISTENCE_MDD_H
#define SERVER_AGENCY_PERSISTENCE_MDD_H

#include <memory>
#include <persistence/options.h>
#include <persistence/storage/client_metrics_storage.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class mod
    {
    public:
        explicit mod(const persistence_config& config);

        ~mod() = default;

        void start();

        client_metrics& clientM_persistence();

    private:
        std::unique_ptr<client_metrics> m_storage;
        const persistence_config& m_config;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
