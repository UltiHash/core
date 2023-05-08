#ifndef SERVER_AGENCY_PERSISTENCE_OPTIONS_H
#define SERVER_AGENCY_PERSISTENCE_OPTIONS_H

#include <options/options.h>


namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    typedef struct
    {
        std::string persistence_path;

    } persistence_config;

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

} // namespace uh::an::persistence

#endif
