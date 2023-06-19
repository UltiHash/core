//
// Created by benjamin-elias on 01.06.23.
//

#include "check_user_license.h"

#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/Exceptions.h>
#include <LicenseSpring/LicenseFileStorage.h>

#include <util/exception.h>

#include <utility>

namespace uh::licensing
{

// ---------------------------------------------------------------------

check_user_license::check_user_license(uh::licensing::license_config license_config,
                                       uh::licensing::api_config apiKey_input,
                                       uh::licensing::credential_config credentialConfig_input,
                                       uh::licensing::license_activate_config license_activate_input)
    :
    check_license(std::move(license_config),
                  std::move(apiKey_input),
                  std::move(credentialConfig_input)),
    m_license_activate(std::move(license_activate_input))
{}

// ---------------------------------------------------------------------

bool check_user_license::valid()
{
    auto licenseId = LicenseSpring::LicenseID::fromUser(m_license_activate.username, m_license_activate.password);

    return licenseRegister(licenseId);
}

} // namespace uh::licensing
