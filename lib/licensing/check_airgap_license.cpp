//
// Created by benjamin-elias on 01.06.23.
//

#include "check_airgap_license.h"

#include <third-party/LicenseSpring/include/LicenseSpring/Configuration.h>
#include <third-party/LicenseSpring/include/LicenseSpring/EncryptString.h>
#include <third-party/LicenseSpring/include/LicenseSpring/LicenseManager.h>
#include <third-party/LicenseSpring/include/LicenseSpring/Exceptions.h>
#include <iostream>

namespace uh::licensing {

    // ---------------------------------------------------------------------

    check_airgap_license::check_airgap_license(std::filesystem::path license_folder, std::string appName,
                                               std::string appVersion) :
            check_license(std::move(license_folder),std::move(appName),
                          std::move(appVersion)){}

    // ---------------------------------------------------------------------

    bool check_airgap_license::valid() {
        //TODO: local implementation if license is valid and exists

        //Collecting network info
        LicenseSpring::ExtendedOptions options;
        options.collectNetworkInfo( true );

        std::shared_ptr<LicenseSpring::Configuration> pConfiguration = LicenseSpring::Configuration::Create(
                EncryptStr("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"), // your LicenseSpring API key (UUID)
                EncryptStr("XXXXXXXXX-XXXXX-XXXXXXXXXXXXX_XXXXXX_XXXXXX"), // your LicenseSpring Shared key
                EncryptStr("XXXXXX"), // product code that you specified in LicenseSpring for your application
                appName, appVersion, options);

        //Key-based implementation
        auto licenseId = LicenseSpring::LicenseID::fromKey("XXXX-XXXX-XXXX-XXXX"); //input license key

        auto licenseManager = LicenseSpring::LicenseManager::create( pConfiguration );

        return licenseRegister(licenseManager, licenseId);
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing