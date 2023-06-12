//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_AIRGAP_LICENSE_H
#define CORE_CHECK_AIRGAP_LICENSE_H

#include "licensing/check_license.h"

namespace uh::licensing {

    class check_airgap_license : public check_license {

    public:

        explicit check_airgap_license(const std::filesystem::path &license_file, std::string apiKey_encrypted,
                                      std::string sharedKey_encrypted,
                                      std::string productId_encrypted, std::string appName = "", std::string appVersion = "");

        bool valid() override;

        void write_license(check_license::role licenseRole, const std::string &app_name_input,
                           const std::string &app_version_input,
                           const std::string &license_key_input);

        std::string check_keygen();

    private:
        std::string keygen;
        const std::string_view keygen_string = "local_license_key: ";
    };

} // uh::licensing

#endif //CORE_CHECK_AIRGAP_LICENSE_H
