#ifndef SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H
#define SERVER_AGENCY_PERSISTENCE_STORAGE_STORAGE_H

#include <persistence/options.h>
#include <protocol/messages.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class client_metrics
    {
    public:
        explicit client_metrics(const persistence_config& config);

        void start();
        void add(const uh::protocol::client_statistics::request& req);
        void flush();

        [[nodiscard]] const std::map<std::string, std::uint64_t>& id_to_size_map() const;

    private:
        void retrieve();
        std::filesystem::path m_target_path;
        std::map<std::string, std::uint64_t> m_id_to_size;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
