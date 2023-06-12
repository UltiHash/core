//
// Created by benjamin-elias on 01.06.23.
//

#include "check_airgap_license.h"

#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/Exceptions.h>

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

        std::string local_keygen_string(keygen_string);
        std::filesystem::path local_license_file_path(license_path);

        auto key_read_func = [&local_keygen_string,&local_license_file_path](){
            std::fstream license_file_stream(local_license_file_path, std::ios_base::in);

            for (std::string line; std::getline(license_file_stream, line);) {
                if (line.starts_with(local_keygen_string)) {
                    return line.substr(local_keygen_string.size(), line.size());
                }
            }

            return std::string{};
        };

        const std::string license_key = key_read_func();

        auto licenseId = LicenseSpring::LicenseID::fromKey(license_key); //input license key

        auto licenseManager = LicenseSpring::LicenseManager::create(pConfiguration);

        return licenseRegister(licenseManager, licenseId);
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing