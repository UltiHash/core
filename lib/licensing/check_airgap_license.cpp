//
// Created by benjamin-elias on 01.06.23.
//

#include "check_airgap_license.h"

#include <third-party/LicenseSpring/include/LicenseSpring/Configuration.h>
#include <third-party/LicenseSpring/include/LicenseSpring/EncryptString.h>
#include <third-party/LicenseSpring/include/LicenseSpring/LicenseManager.h>
#include <third-party/LicenseSpring/include/LicenseSpring/Exceptions.h>

#include <utility>

namespace uh::licensing {

    // ---------------------------------------------------------------------

    check_airgap_license::check_airgap_license(std::filesystem::path license_folder, std::string appName,
                                               std::string appVersion,
                                               std::string apiKey, std::string sharedKey, std::string productId) :
            check_license(std::move(license_folder), std::move(appName),
                          std::move(appVersion), std::move(apiKey), std::move(sharedKey),
                          std::move(productId)) {}

    // ---------------------------------------------------------------------

    bool check_airgap_license::valid()
    {
        //Collecting network info
        LicenseSpring::ExtendedOptions options;
        options.collectNetworkInfo(true);

        const std::string apiKey_crypt = LicenseSpring::Xor_string<36,char>(this->apiKey.c_str())._string;
        const std::string sharedKey_crypt = LicenseSpring::Xor_string<43,char>(this->sharedKey.c_str())._string;
        const std::string productId_crypt = LicenseSpring::Xor_string<6,char>(this->productId.c_str())._string;

        std::shared_ptr<LicenseSpring::Configuration> pConfiguration = LicenseSpring::Configuration::Create(
                apiKey_crypt, // your LicenseSpring API key (UUID)
                sharedKey_crypt, // your LicenseSpring Shared key
                productId_crypt, // product code that you specified in LicenseSpring for your application
                appName, appVersion, options);

        //Key-based implementation
        auto licenseId = LicenseSpring::LicenseID::fromKey("XXXX-XXXX-XXXX-XXXX"); //input license key

        auto licenseManager = LicenseSpring::LicenseManager::create(pConfiguration);

        return licenseRegister(licenseManager, licenseId);
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing