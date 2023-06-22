//
// Created by benjamin-elias on 15.06.23.
//

#include "options.h"

#include "config.hpp"

#include "util/exception.h"
#include "logging/logging_boost.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace uh::dbn::licensing
{

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map &vars)
{
    m_config.license_key = std::filesystem::path(vars["activate"].as<std::string>());
    m_config.license_type = "AirgapKeyOnline";

    if (std::any_of(m_config.licensing_path.cbegin(), m_config.licensing_path.cend(),
                    [](auto c)
                    { return c == ';'; }))
    {
        std::vector<std::string> tmp_vec_configs;
        boost::split(tmp_vec_configs, m_config.license_key, boost::is_any_of(";"));

        switch (tmp_vec_configs.size())
        {
            case 2:m_config.license_type = tmp_vec_configs[0];
                m_config.license_key = tmp_vec_configs[1];
            case 3:m_config.license_type = tmp_vec_configs[0];
                m_config.license_key = tmp_vec_configs[1];
                m_config.licensing_path = tmp_vec_configs[2];
                break;
            case 4:m_config.license_type = tmp_vec_configs[0];
                m_config.license_user = tmp_vec_configs[1];
                m_config.license_password = tmp_vec_configs[2];
                m_config.licensing_path = tmp_vec_configs[3];
                break;
            default:
                std::string err_string = "Received string was: " + m_config.license_key +
                    "Activation command was not populated with either "
                    "\"license key\", "
                    "\"license type;license key\", "
                    "\"license type;license_key;license-path\" or "
                    "\"license type;username;password;license-path\"."
                    " Default license path is /var/lib ."
                    " Default license type is AirgapKeyOnline";
                INFO << err_string;
                THROW(util::exception, err_string);
        }
    }

    if (m_config.licensing_path.empty())
    {
        m_config.licensing_path = "/var/lib/" + std::string(PROJECT_NAME) + "/licensing";
        std::filesystem::create_directory(m_config.licensing_path);
    }
    else
    {
        if (!std::filesystem::exists(m_config.licensing_path))
            throw std::runtime_error("Path doesn't exist: " + m_config.licensing_path);

        if (std::filesystem::is_directory(m_config.licensing_path))
        {
            if (access(m_config.licensing_path.c_str(), W_OK) != 0)
                throw std::runtime_error("User doesn't have write permission on: " + m_config.licensing_path);
        }
        else
            throw std::runtime_error("Path '" + m_config.licensing_path + "' is not a directory.");
    }

    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing
