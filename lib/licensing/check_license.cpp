//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_license.h"
#include "util/exception.h"
#include "logging/logging_boost.h"
#include "io/temp_file.h"

#include <fstream>
#include <string>
#include <iostream>
#include <utility>

namespace uh::licensing {

    // ---------------------------------------------------------------------

    check_license::check_license(std::filesystem::path license_path, check_license::license_type license_type,
                                 std::string apiKey_encrypted, std::string sharedKey_encrypted, std::string productId_enrypted, std::string appName,
                                 std::string appVersion) :
            license_path(std::move(license_path)), licenseTypeInternal(license_type),
            appName(std::move(appName)), appVersion(std::move(appVersion)),
            apiKey_crypt(std::move(apiKey_encrypted)), sharedKey_crypt(std::move(sharedKey_encrypted)), productId_crypt(std::move(productId_enrypted))
            {
        if(this->appName.empty())
            this->appName = check_app_name();

        if(this->appVersion.empty())
            this->appVersion = check_app_version();
    }

    // ---------------------------------------------------------------------

    check_license::role check_license::check_role()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(role_string)) {
                line = line.substr(role_string.size(), line.size());

                if (line == "agency_node")
                    return check_license::role::AGENCY_NODE;

                if (line == "data_node")
                    return check_license::role::DATA_NODE;
            }
        }

        return check_license::role::THROW_ROLE;
    }

    // ---------------------------------------------------------------------

    check_license::license_type check_license::check_license_type()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(license_type_string)) {
                line = line.substr(license_type_string.size(), line.size());

                if (line == airgap_license_string)
                    return check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION;

                if (line == floating_license_string)
                    return check_license::license_type::FLOATING_ONLINE_USER_LICENSE;
            }
        }

        return check_license::license_type::THROW_LICENSE_TYPE;
    }

    // ---------------------------------------------------------------------

    std::string check_license::check_app_name()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(appName_string)) {
                line = line.substr(appName_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

    std::string check_license::check_app_version()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(appVersion_string)) {
                line = line.substr(appVersion_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

    bool check_license::valid() {
        return true;
    }

    // ---------------------------------------------------------------------

    io::file check_license::write_license_file(check_license::role licenseRole, const std::string &app_name_input,
                                               const std::string &app_version_input)
            {
        std::string role_set_string, license_type_set_string;

        switch (licenseRole) {
            case role::AGENCY_NODE:
                role_set_string = "agency_node";
                break;
            case role::DATA_NODE:
                role_set_string = "data_node";
                break;
            default:
                THROW(util::exception, "No license role detected!");
        }

        switch (licenseTypeInternal) {
            case license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION:
                license_type_set_string = airgap_license_string;
                break;
            case license_type::FLOATING_ONLINE_USER_LICENSE:
                license_type_set_string = floating_license_string;
                break;
            default:
                THROW(util::exception, "No license type detected!");
        }

        std::filesystem::path out_license_path = license_path / (std::string(role_set_string) + ".lic");

        if (std::filesystem::exists(out_license_path))
            THROW(util::exception, "A license already existed on path \"" + license_path.string() + "\" !");

        {
            std::filesystem::path write_tmp_path = license_path;
            if(write_tmp_path.extension() == ".lic")
                write_tmp_path = write_tmp_path.parent_path();
            io::temp_file write_temp(write_tmp_path);

            write_temp.write(std::string(appName_string) + app_name_input + "\n");
            write_temp.write(std::string(appVersion_string) + app_version_input + "\n");
            write_temp.write(std::string(role_string) + role_set_string + "\n");
            write_temp.write(std::string(license_type_string) + license_type_set_string + "\n");

            if(std::filesystem::is_directory(license_path)){
                license_path = license_path / (role_set_string + ".lic");
                write_temp.release_to(license_path);
            }

            else {
                if(license_path.extension() != ".lic")
                    license_path += ".lic";
                write_temp.release_to(license_path);
            }
        }

        io::file out_file(license_path,std::ios_base::app);

        return out_file;
    }

    // ---------------------------------------------------------------------

    bool check_license::is_written() {
        return std::filesystem::exists(license_path) and std::filesystem::is_regular_file(license_path);
    }

    // ---------------------------------------------------------------------

    const std::filesystem::path &check_license::getLicensePath() const {
        return license_path;
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing