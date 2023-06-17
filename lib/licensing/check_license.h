//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <filesystem>
#include <utility>
#include <map>

#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/LicenseID.h>

#include <io/temp_file.h>

#include "boost/property_tree/json_parser.hpp"

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
         * @param license_directory is the path where a license file is stored
         */
        explicit check_license(std::filesystem::path license_directory,
                               check_license::license_type license_type,
                               std::string apiKey_encrypted,
                               std::string sharedKey_encrypted,
                               std::string productId_enrypted,
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

        /**
         *
         * @return a map of key value pairs that will configure the application
         */
        virtual std::map<std::string, std::string>
        getCustomAndFeatureFields(){
            return {};
        };

    protected:
        std::filesystem::path license_path;
        bool replace_license = false;

        io::file
        write_license_file(check_license::role licenseRole, const std::string &app_name_input,
                           const std::string &app_version_input);

        const std::string_view role_string = "server_role: ";
        const std::string_view license_type_string = "license_type: ";

        const std::string_view appName_string = "app_name: ";
        const std::string_view appVersion_string = "app_version: ";

        const std::string_view airgap_license_string = "airgap_license_with_online_activation";
        const std::string_view floating_license_string = "floating_online_user_license";

        // ---------------------------------------------------------------------

        /**
         * License Spring info and functions
         *
         * apiKey_crypt is 36 characters long
         * sharedKey_crypt is 43 characters long
         * productId_crypt is 6 characters long
         */
        std::string appName,
        appVersion,
        apiKey_crypt,
        sharedKey_crypt,
        productId_crypt;

        const license_type licenseTypeInternal;

        const bool collectNetworkInfo = true;
        const bool enableLogging = true;
        const bool enableVMDetection = false;

        /**
         *
         * @param licenseManager parsed license file and API authentication
         * @param licenseId license sign method either user based or key based
         * @return success or failure of license activation and check
         */
        static bool licenseRegister(const std::shared_ptr<LicenseSpring::LicenseManager> &licenseManager,
                                    const LicenseSpring::LicenseID& licenseId);

        /**
         *
         * @param license is the incoming parsed license file
         * @return if the license was still valid
         */
        static bool license_check(const LicenseSpring::License::ptr_t& license);

        /**
         *
         * @param licenseManager contains a storage method to find the license file
         * @return a map of key value pairs that will configure the application
         */
        static std::map<std::string, std::string>
        getCustomAndFeatureFields(const std::shared_ptr<LicenseSpring::LicenseManager> &licenseManager);

        // ---------------------------------------------------------------------

    };

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
