//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <filesystem>
#include <utility>

namespace uh::licensing{

    class check_license{

    public:

        /**
         *
         * Check if `role` is enabled in the configured license.
         * @throw if `role` does not match given license
         */
        enum class role: unsigned char{
            THROW_ROLE,
            AGENCY_NODE,
            DATA_NODE
        };

        /**
         *
         * Check the type of a license file
         * @throw if `license_type` is not described in the license file
         */
        enum class license_type: unsigned char{
            THROW_LICENSE_TYPE,
            AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION,
            FLOATING_ONLINE_USER_LICENSE
        };

        /**
         * manages license file creation and parsing
         * First line license: license role
         * Second line license:
         *
         * @param license_folder is the folder where a license file is stored
         */
        explicit check_license(std::filesystem::path license_folder, std::string appName, std::string appVersion):
        license_path(std::move(license_folder)), appName(std::move(appName)), appVersion(std::move(appVersion)){};

        /**
         *
         * @return role that was initialized by license file
         */
        role check_role();

        /**
         *
         * @return type of license handling
         */
        license_type check_license_type();

        /**
         * the default license is not timed
         *
         * @param license_path is the path to the license file
         * @return if the license file is valid for the implemented service role and features
         */
        virtual bool valid();

    protected:
        std::filesystem::path license_path;

        void write_license_base(role licenseRole, license_type licenseType);

        const std::string role_string = "server_role: ";
        const std::string license_type_string = "license_type: ";

        std::string appName, appVersion;
    };

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
