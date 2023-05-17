#ifndef SERVER_AGENCY_PERSISTENCE_OPTIONS_H
#define SERVER_AGENCY_PERSISTENCE_OPTIONS_H

#include <options/options.h>
#include <filesystem>
#include "options/persistence_options.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class options : public uh::options::persistence_options
    {
    public:
        options() = default;

        uh::options::action evaluate(const boost::program_options::variables_map& vars) override;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
