#ifndef SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H
#define SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H

#include <logging/logging_boost.h>
#include <persistence/options.h>
#include "persist.h"
#include <io/temp_file.h>
#include <protocol/messages.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class client_metrics : public persist
    {
    public:
        explicit client_metrics(const persistence_config& config);

        void start() override;
        void add(const uh::protocol::client_statistics::request& req) override;
        void serialize();
        void deserialize();

    private:
        std::filesystem::path m_target_path;
        std::map<std::string, std::uint64_t> m_id_to_size;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
