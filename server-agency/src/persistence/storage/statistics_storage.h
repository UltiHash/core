#ifndef SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H
#define SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H

#include "persistence/storage.h"
#include <logging/logging_boost.h>
#include <persistence/options.h>
#include <io/temp_file.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class statistics_storage : public storage
    {
    public:
        explicit statistics_storage(const persistence_config& config);

        void start() override;

    private:
        std::filesystem::path m_target_path;
        std::unique_ptr<io::temp_file> m_tmp {};
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
