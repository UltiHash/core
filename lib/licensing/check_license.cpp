//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_license.h"

#include <fstream>
#include <string>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_license::role check_license::check_role()
    {
        std::fstream license_file_stream(license_path,std::ios_base::in);

        const std::string role_string = "server_role: ";

        for(std::string line; std::getline(license_file_stream,line);)
        {
            if(line.starts_with(role_string))
            {
                line = line.substr(role_string.size(),line.size());

                if(line == "agency_node")
                    return check_license::role::AGENCY_NODE;

                if(line == "data_node")
                    return check_license::role::DATA_NODE;
            }
        }

        return check_license::role::THROW_ROLE;
    }

    // ---------------------------------------------------------------------

    check_license::license_type check_license::check_license_type()
    {
        std::fstream license_file_stream(license_path,std::ios_base::in);

        const std::string role_string = "license_type: ";

        for(std::string line; std::getline(license_file_stream,line);)
        {
            if(line.starts_with(role_string))
            {
                line = line.substr(role_string.size(),line.size());

                if(line == "airgap_license_with_online_activation")
                    return check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION;

                if(line == "floating_user_license")
                    return check_license::license_type::FLOATING_USER_LICENSE;
            }
        }

        return check_license::license_type::THROW_LICENSE_TYPE;
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing