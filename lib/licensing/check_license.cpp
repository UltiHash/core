//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_license.h"
#include "util/exception.h"

#include <fstream>
#include <string>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_license::role check_license::check_role()
    {
        std::fstream license_file_stream(license_path,std::ios_base::in);

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

        for(std::string line; std::getline(license_file_stream,line);)
        {
            if(line.starts_with(license_type_string))
            {
                line = line.substr(license_type_string.size(),line.size());

                if(line == "airgap_license_with_online_activation")
                    return check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION;

                if(line == "floating_user_license")
                    return check_license::license_type::FLOATING_ONLINE_USER_LICENSE;
            }
        }

        return check_license::license_type::THROW_LICENSE_TYPE;
    }

    bool check_license::valid() {
        return true;
    }

    void check_license::write_license_base(check_license::role licenseRole, check_license::license_type licenseType)
    {
        std::string role_set_string, license_type_set_string;

        switch (licenseRole)
        {
            case role::AGENCY_NODE:
                role_set_string = "agency_node";
                break;
            case role::DATA_NODE:
                role_set_string = "data_node";
                break;
            default:
                THROW(util::exception,"No license role detected!");
        }

        switch (licenseType)
        {
            case license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION:
                license_type_set_string = "airgap_license_with_online_activation";
                break;
            case license_type::FLOATING_ONLINE_USER_LICENSE:
                license_type_set_string = "floating_online_user_license";
                break;
            default:
                THROW(util::exception,"No license type detected!");
        }

        std::filesystem::path out_license_path = license_path / (role_string + ".lic");

        if(std::filesystem::exists(out_license_path))
            THROW(util::exception,"A license already existed on path \""+license_path.string()+"\" !");

        std::fstream license_file_stream(license_path,std::ios_base::out);

        license_file_stream << role_string << role_set_string << std::endl;
        license_file_stream << license_type_string << license_type_set_string << std::endl;
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing