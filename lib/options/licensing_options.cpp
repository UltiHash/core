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
                ("activate", value<std::string>()->default_value(""),
                "Enter: \"license key\" or "
                "\"license type;license key\" or "
                "\"license type;license_key;license-path\" or "
                "\"license type;username;password;license-path\" to activate license on first run."
                " Default license path DIRECTORY is /var/lib/***-node/licensing ."
                " Default license type is AirgapKeyOnline");
        visible().add_options()
                ("activate-replace", value<std::string>()->default_value(""),
                 "Enter: \"license key\" or "
                 "\"license type;license key\" or "
                 "\"license type;license_key;license-path\" or "
                 "\"license type;username;password;license-path\" to activate license "
                 "and replace/DELETE old license files."
                 " Default license path DIRECTORY is /var/lib/***-node/licensing ."
                 " Default license type is AirgapKeyOnline");
    }

// ---------------------------------------------------------------------

    const licensing_config& licensing_options::config() const
    {
        return m_config;
    }

// ---------------------------------------------------------------------

} // options