//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <filesystem>
#include <utility>

#include <io/temp_file.h>

#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/LicenseID.h>
#include <LicenseSpring/EncryptString.h>

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
         * @param license_path is the path where a license file is stored
         */
        explicit check_license(const std::filesystem::path &license_path, check_license::license_type license_type,
                               std::string apiKey, std::string sharedKey, std::string productId,
                               std::string appName = "",
                               std::string appVersion = "");

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
         *
         * @return name of app
         */
        std::string check_app_name();

        /**
         *
         * @return name of app
         */
        std::string check_app_version();

        /**
         * the default license is not timed
         *
         * @param license_path is the path to the license file
         * @return if the license file is valid for the implemented service role and features
         */
        virtual bool valid();

        /**
         *
         * @return if license file representation is written to disk
         */
        bool is_written();

        /**
         *
         * @return license path
         */
        [[nodiscard]] const std::filesystem::path &getLicensePath() const;

    protected:
        std::filesystem::path license_path;

        io::file
        write_license_file(check_license::role licenseRole, const std::string &app_name_input,
                           const std::string &app_version_input);

        const std::string_view role_string = "server_role: ";
        const std::string_view license_type_string = "license_type: ";

        const std::string_view appName_string = "app_name: ";
        const std::string_view appVersion_string = "app_version: ";

        /**
         * apiKey is 36 characters long
         * sharedKey is 43 characters long
         * productId is 6 characters long
         */
        std::string appName, appVersion, apiKey, sharedKey, productId;

        const license_type licenseTypeInternal;

        static int licenseRegister(const std::shared_ptr<LicenseSpring::LicenseManager> &licenseManager,
                            const LicenseSpring::LicenseID& licenseId);
    };

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
