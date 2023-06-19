//
// Created by benjamin-elias on 01.06.23.
//

#include "check_key_license.h"

#include <utility>

namespace uh::licensing
{

// ---------------------------------------------------------------------

check_key_license::check_key_license(uh::licensing::license_config license_config,
                                     uh::licensing::api_config apiKey_input,
                                     uh::licensing::credential_config credentialConfig_input,
                                     uh::licensing::license_activate_config license_activate_input)
    :
    check_license(
        std::move(license_config),
        std::move(apiKey_input),
        std::move(credentialConfig_input)),
    m_license_activate(std::move(license_activate_input))
{}

// ---------------------------------------------------------------------

bool check_key_license::valid()
{
    auto licenseId = LicenseSpring::LicenseID::fromKey(m_license_activate.key);

    return licenseRegister(licenseId);
}

} // namespace uh::licensing