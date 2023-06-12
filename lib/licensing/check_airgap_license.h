//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_AIRGAP_LICENSE_H
#define CORE_CHECK_AIRGAP_LICENSE_H

#include "licensing/check_license.h"

namespace uh::licensing {

    class check_airgap_license : public check_license {

    public:

        explicit check_airgap_license(std::filesystem::path license_folder, std::string appName, std::string appVersion,
                                      std::string apiKey, std::string sharedKey, std::string productId);

        bool valid() override;

    private:
        const std::string_view keygen_string = "local_license_key: ";
    };

} // uh::licensing

#endif //CORE_CHECK_AIRGAP_LICENSE_H
