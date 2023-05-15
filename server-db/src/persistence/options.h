#ifndef SERVER_DATABASE_PERSISTENCE_OPTIONS_H
#define SERVER_DATABASE_PERSISTENCE_OPTIONS_H

#include <options/options.h>
#include <filesystem>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

    struct persistence_config
    {
        std::string persistence_path;
    };

// ---------------------------------------------------------------------

    class options : public uh::options::options
    {
    public:
        options();

        uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

        [[nodiscard]] const persistence_config& config() const;

    private:
        persistence_config m_config;

    };

// ---------------------------------------------------------------------

} // namespace uh::dbn:persistence

#endif
