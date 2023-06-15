//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_ONLINE_LICENSE_H
#define CORE_CHECK_ONLINE_LICENSE_H

#include "licensing/check_license.h"

#include <utility>

namespace uh::licensing {

    class check_online_license : public check_license {

    public:

        /*
         * license model of online activation and floating use with a time limit set on license cloud
         */
        explicit check_online_license(const std::filesystem::path &license_file, std::string apiKey_encrypted,
                                      std::string sharedKey_encrypted,
                                      std::string productId_encrypted, std::string appName = "",
                                      std::string appVersion = "",
                                      std::string userName = "", std::string password = "");

        /**
         *
         * @return if the license key and activation could be validated
         */
        bool valid() override;

        /**
         * write a license file
         *
         * @param licenseRole what role does the application play in the setup
         * @param app_name_input application name
         * @param app_version_input application version
         * @param username_input application license validation username
         * @param password_input application license validation password
         */
        void write_license(check_license::role licenseRole, const std::string &app_name_input,
                           const std::string &app_version_input,
                           const std::string &username_input, const std::string &password_input);

        /**
         *
         * @return UltiHash username string from UltiHash license file
         */
        std::string check_user_name();

        /**
         *
         * @return UltiHash password string from UltiHash license file
         */
        std::string check_password();

        /**
         *
         * @return license specific key value pairs for features, product variables and limits
         */
        std::map<std::string, std::string>
            getCustomAndFeatureFields() override;

    private:
        std::string userName{}, password{};
        const std::string_view user_name_string = "user_name: ";
        const std::string_view password_string = "password: ";
    };

} // namespace uh::licensing

#endif //CORE_CHECK_ONLINE_LICENSE_H
