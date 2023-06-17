//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_OPTIONS_H
#define CORE_OPTIONS_H

#include <options/options.h>
#include <filesystem>
#include <options/licensing_options.h>

namespace uh::an::licensing {


// ---------------------------------------------------------------------

    class options : public uh::options::licensing_options
    {
    public:
        options() = default;

        uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

    };

// ---------------------------------------------------------------------

} // namespace uh::an::licensing

#endif //CORE_OPTIONS_H
