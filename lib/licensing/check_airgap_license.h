//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_AIRGAP_LICENSE_H
#define CORE_CHECK_AIRGAP_LICENSE_H

#include "licensing/check_license.h"

namespace uh::licensing {

    class check_airgap_license : public check_license {

    public:

        /*
         * license model of online activation and air-gap use without time limit
         */
        explicit check_airgap_license(const std::filesystem::path &license_directory,
                                      std::string apiKey_encrypted,
                                      std::string sharedKey_encrypted,
                                      std::string productId_encrypted,
                                      std::string appName = "",
                                      std::string appVersion = "",
                                      bool replace_license = false);

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
         * @param license_key_input application license key
         */
        void write_license(check_license::role licenseRole, const std::string &app_name_input,
                           const std::string &app_version_input,
                           const std::string &license_key_input);

        /**
         *
         * @return UltiHash license string from UltiHash license file
         */
        std::string check_keygen();

        /**
         *
         * @return license specific key value pairs for features, product variables and limits
         */
        std::map<std::string, std::string>
                getCustomAndFeatureFields() override;

    private:
        std::string keygen;
        const std::string_view keygen_string = "local_license_key: ";
    };

} // uh::licensing

#endif //CORE_CHECK_AIRGAP_LICENSE_H
