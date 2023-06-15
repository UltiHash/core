#ifndef SERVER_AGENCY_STATE_CLIENT_METRICS_STATE_H
#define SERVER_AGENCY_STATE_CLIENT_METRICS_STATE_H

#include <state/options.h>
#include <protocol/messages.h>

namespace uh::an::state
{

// ---------------------------------------------------------------------

    class client_metrics_state
    {
    public:
        explicit client_metrics_state(const storage_config& config);

        void start();
        void stop();

        void add(const uh::protocol::client_statistics::request& req);
        void flush();

        [[nodiscard]] const std::map<std::string, std::uint64_t>& id_to_size_map() const;

    private:
        void retrieve();
        std::filesystem::path m_target_path;
        std::map<std::string, std::uint64_t> m_id_to_size;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::state

#endif
