//
// Created by benjamin-elias on 15.06.23.
//

#include "licensing_options.h"

using namespace boost::program_options;

namespace uh::options {
    
// ---------------------------------------------------------------------

    licensing_options::licensing_options(): uh::options::options("Licensing Options")
    {
        visible().add_options()
                ("activate", value<std::string>()->default_value("/var/lib"),
                        "Path where license file information can be stored permanently");
    }

// ---------------------------------------------------------------------

    const licensing_config& licensing_options::config() const
    {
        return m_config;
    }

// ---------------------------------------------------------------------

} // options