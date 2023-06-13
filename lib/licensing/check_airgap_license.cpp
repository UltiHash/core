//
// Created by benjamin-elias on 01.06.23.
//

#include "check_airgap_license.h"

#include <utility>

namespace uh::licensing {

    // ---------------------------------------------------------------------

    check_airgap_license::check_airgap_license(const std::filesystem::path &license_file, std::string apiKey_encrypted,
                                               std::string sharedKey_encrypted,
                                               std::string productId_encrypted, std::string appName, std::string appVersion) :
            check_license(license_file, license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION,
                          std::move(apiKey_encrypted),
                          std::move(sharedKey_encrypted),
                          std::move(productId_encrypted), std::move(appName),
                          std::move(appVersion))
                          {
        if(this->keygen.empty())
          this->keygen = check_keygen();
    }

    // ---------------------------------------------------------------------

    bool check_airgap_license::valid()
    {
        return keygen == check_keygen();
    }

    // ---------------------------------------------------------------------

    void check_airgap_license::write_license(check_license::role licenseRole, const std::string &app_name_input,
                                             const std::string &app_version_input,
                                             const std::string &license_key_input)
                                             {
        auto out_file = write_license_file(licenseRole, app_name_input, app_version_input);
        out_file.write(std::string(keygen_string) + license_key_input + "\n");
    }

    // ---------------------------------------------------------------------

    std::string check_airgap_license::check_keygen()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(keygen_string)) {
                line = line.substr(keygen_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing