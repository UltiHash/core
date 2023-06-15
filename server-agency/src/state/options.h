#ifndef SERVER_AGENCY_PERSISTENCE_OPTIONS_H
#define SERVER_AGENCY_PERSISTENCE_OPTIONS_H

#include <options/options.h>
#include <filesystem>

namespace uh::an::state
{

    struct storage_config
    {
        constexpr static std::string_view default_data_directory = "/var/lib/uh-agency-node";

        std::filesystem::path data_directory = default_data_directory;
        std::filesystem::path an_metrics = std::string(default_data_directory) + std::string("state");
    };

// ---------------------------------------------------------------------

    class options : public uh::options::options
    {
    public:
        options();

        uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

        [[nodiscard]] const storage_config& config() const;

    private:
        storage_config m_config;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
