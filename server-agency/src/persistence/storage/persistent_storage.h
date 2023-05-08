#ifndef SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H
#define SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H

#include "persistence/storage.h"
#include <logging/logging_boost.h>
#include <persistence/options.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class persistent_storage : public storage
    {
    public:
        persistent_storage(const persistence_config &config);

        void start() override;

    private:
        const persistence_config &m_config;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
