//
// Created by benjamin-elias on 15.06.23.
//

#include "options.h"

namespace uh::dbn::licensing {
    
// ---------------------------------------------------------------------

    uh::options::action options::evaluate(const boost::program_options::variables_map& vars) {
        m_config.licensing_path = std::filesystem::path(vars["activate"].as<std::string>());

        if (m_config.licensing_path == "/var/lib") {
            m_config.licensing_path = "/var/lib/data-node/licensing";
            std::filesystem::create_directory(m_config.licensing_path);
        } else {
            if (!std::filesystem::exists(m_config.licensing_path))
                throw std::runtime_error("Path doesn't exist: " + m_config.licensing_path);

            if (std::filesystem::is_directory(m_config.licensing_path)) {
                if (access(m_config.licensing_path.c_str(), W_OK) != 0)
                    throw std::runtime_error("User doesn't have write permission on: " + m_config.licensing_path);
            } else
                throw std::runtime_error("Path '" + m_config.licensing_path + "' is not a directory.");
        }

        return uh::options::action::proceed;
    }

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing
