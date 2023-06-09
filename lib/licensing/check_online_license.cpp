//
// Created by benjamin-elias on 01.06.23.
//

#include "check_online_license.h"

#include <third-party/LicenseSpring/include/LicenseSpring/Configuration.h>
#include <third-party/LicenseSpring/include/LicenseSpring/EncryptString.h>
#include <third-party/LicenseSpring/include/LicenseSpring/LicenseManager.h>
#include <third-party/LicenseSpring/include/LicenseSpring/Exceptions.h>

#include <utility>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_online_license::check_online_license(std::filesystem::path license_folder, std::string appName,
                                               std::string appVersion,
                                               std::string userName, std::string password) :
            check_license(std::move(license_folder), std::move(appName),
                          std::move(appVersion), std::string(), std::string(), std::string()), userName(std::move(userName)), password(std::move(password)){}

    // ---------------------------------------------------------------------

    bool check_online_license::valid() {
        //TODO: online implementation if license is valid and exists

        //Collecting network info
        LicenseSpring::ExtendedOptions options;
        options.collectNetworkInfo( true );

        std::shared_ptr<LicenseSpring::Configuration> pConfiguration = LicenseSpring::Configuration::Create(
                EncryptStr("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"), // your LicenseSpring API key (UUID)
                EncryptStr("XXXXXXXXX-XXXXX-XXXXXXXXXXXXX_XXXXXX_XXXXXX"), // your LicenseSpring Shared key
                EncryptStr("XXXXXX"), // product code that you specified in LicenseSpring for your application
                appName, appVersion, options);

        //For user-based implementation comment out above line, and use bottom 3 lines
        auto licenseId = LicenseSpring::LicenseID::fromUser( userName, password );

        // User-based implementation
        auto licenseManager = LicenseSpring::LicenseManager::create( pConfiguration );

        return licenseRegister(licenseManager, licenseId);
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing
