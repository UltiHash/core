#ifndef OPTIONS_PERSISTENCE_OPTIONS_H
#define OPTIONS_PERSISTENCE_OPTIONS_H

#include "options/options.h"
#include <filesystem>

namespace uh::options
{

// ---------------------------------------------------------------------

    struct persistence_config
    {
        std::string persistence_path;
    };

// ---------------------------------------------------------------------

    class persistence_options : public uh::options::options
    {
    public:
        persistence_options();

        [[nodiscard]] const persistence_config& config() const;

    protected:
        persistence_config m_config;

    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
