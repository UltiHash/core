#ifndef SERVER_AGENCY_PERSISTENCE_MDD_H
#define SERVER_AGENCY_PERSISTENCE_MDD_H

#include <memory>
#include <persistence/storage.h>
#include <persistence/options.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class mod
    {
    public:
        explicit mod(const persistence_config& config);

        ~mod() = default;

        void start();

        storage& client_storage();

    private:
        std::unique_ptr<storage> m_storage;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
