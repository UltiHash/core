//
// Created by benjamin-elias on 01.06.23.
//

#include "check_online_license.h"

#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/Exceptions.h>

#include <utility>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_online_license::check_online_license(std::filesystem::path license_folder, std::string appName,
                                               std::string appVersion,
                                               std::string userName, std::string password, std::string apiKey,
                                               std::string sharedKey,
                                               std::string productId) :
            check_license(std::move(license_folder), std::move(appName),
                          std::move(appVersion), std::move(apiKey), std::move(sharedKey),
                          std::move(productId)),
                          userName(std::move(userName)), password(std::move(password)){}

    // ---------------------------------------------------------------------

    bool check_online_license::valid()
    {
        //Collecting network info
        LicenseSpring::ExtendedOptions options;
        options.collectNetworkInfo( true );

        const std::string apiKey_crypt = LicenseSpring::Xor_string<36,char>(this->apiKey.c_str())._string;
        const std::string sharedKey_crypt = LicenseSpring::Xor_string<43,char>(this->sharedKey.c_str())._string;
        const std::string productId_crypt = LicenseSpring::Xor_string<6,char>(this->productId.c_str())._string;

        std::shared_ptr<LicenseSpring::Configuration> pConfiguration = LicenseSpring::Configuration::Create(
                apiKey_crypt, // your LicenseSpring API key (UUID)
                sharedKey_crypt, // your LicenseSpring Shared key
                productId_crypt, // product code that you specified in LicenseSpring for your application
                appName, appVersion, options);

        //For user-based implementation comment out above line, and use bottom 3 lines
        auto licenseId = LicenseSpring::LicenseID::fromUser( userName, password );

        // User-based implementation
        auto licenseManager = LicenseSpring::LicenseManager::create( pConfiguration );

        return licenseRegister(licenseManager, licenseId);
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing
