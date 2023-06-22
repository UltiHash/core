//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_airgap_license.h"
#include "util/exception.h"
#include "logging/logging_boost.h"
#include "io/temp_file.h"
#include "io/sha512.h"

#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/Exceptions.h>

#include <license_spring_credentials.h>

#include <string>
#include <iostream>
#include <utility>

#include "boost/property_tree/ptree.hpp"
#include "boost/algorithm/string.hpp"

using namespace LicenseSpring;

namespace uh::licensing
{

namespace
{

// ---------------------------------------------------------------------

std::shared_ptr<LicenseSpring::Configuration> mk_config(
    const std::string& product,
    const std::string& appName,
    const std::string& appVersion)
{
    LicenseSpring::ExtendedOptions options;

#ifdef DEBUG
    options.enableLogging(true);
#endif
    options.enableSSLCheck(true);

    return LicenseSpring::Configuration::Create(
        EncryptStr(LICENSE_SPRING_API_KEY),
        EncryptStr(LICENSE_SPRING_SHARED_KEY),
        product,
        appName,
        appVersion,
        options);
}

// ---------------------------------------------------------------------

}

// ---------------------------------------------------------------------

check_airgap_license::check_airgap_license(uh::licensing::license_config license_config,
                                           uh::licensing::api_config apiKey_input,
                                           uh::licensing::credential_config credentialConfig_input,
                                           uh::licensing::license_activate_config license_activate_input)
    : m_id(LicenseSpring::LicenseID::fromKey(license_activate_input.key)),
      m_config(mk_config(apiKey_input.productId,
                         credentialConfig_input.appName,
                         credentialConfig_input.appVersion)),
      m_storage(LicenseSpring::LicenseFileStorage::create(license_config.license_path.wstring())),
      m_manager(LicenseSpring::LicenseManager::create(m_config, m_storage)),
      m_license(m_manager->activateLicense(m_id))
{
    reload(*m_license);
}

// ---------------------------------------------------------------------

bool check_airgap_license::valid()
{
    return true;
}

// ---------------------------------------------------------------------

bool check_airgap_license::has_feature(feature f) const
{
    return m_features.contains(f);
}

// ---------------------------------------------------------------------

std::string check_airgap_license::feature_arg_string(feature f, const std::string& name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end())
    {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::string>(name);
}

// ---------------------------------------------------------------------

std::size_t check_airgap_license::feature_arg_size_t(feature f, const std::string& name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end())
    {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::size_t>(name);
}

// ---------------------------------------------------------------------

void check_airgap_license::reload(LicenseSpring::License& license)
{
    std::map<feature, boost::property_tree::ptree> features;

    for (const auto& feature : license.features())
    {
        try
        {
            auto feat = feature_from_string(feature.name());
            auto& ptree = features[feat];

            auto md = feature.metadata();
            if (!md.empty())
            {
                std::stringstream metadata(md);
                boost::property_tree::read_json(metadata, features[feat]);
            }
        }
        catch (const util::exception& e)
        {
            continue;
        }
    }

    std::swap(features, m_features);
}

// ---------------------------------------------------------------------

#if 0
bool check_airgap_license::license_check(const LicenseSpring::License::ptr_t &license)
{
    //First we'll run a online check. This will check your license on the
    //LicenseSpring servers, and sync up your local license to match your online
    if (license != nullptr)
    {
        try
        {
            INFO << "Checking license online..." << std::endl;
            license->check();
            INFO << "License successfully checked" << std::endl;
        }
        catch (LicenseSpring::LicenseStateException)
        {
            WARNING << "Online license is not valid" << std::endl;
            if (!license->isActive())
            {
                ERROR << "License is inactive" << std::endl;
                return false;
            }
            if (license->isExpired())
            {
                ERROR << "License is expired" << std::endl;
                return false;
            }
            if (!license->isEnabled())
            {
                ERROR << "License is disabled" << std::endl;
                return false;
            }
        }
        catch (std::exception &e)
        {
            ERROR << e.what();
            return false;
        }

        //We use localCheck() in LicenseSpring/License.h in the include folder.This is
        //useful to check if the license hasn't been copied over from another device, and
        //that the license is still valid. Note: valid and active are not the same thing. See
        //tutorial [link here] to learn the difference.
        try
        {
            INFO << "License successfully loaded, performing local check of the license..." << std::endl;
            license->localCheck();
            INFO << "Local validation successful" << std::endl;
        }
        catch (LicenseSpring::LocalLicenseException)
        { //Exception if we cannot read the local license or the local license file is corrupt
            ERROR << "Could not read previous local license. Local license may be corrupt. "
                  << "Create a new local license by activating your license." << std::endl;
            return false;
        }
        catch (LicenseSpring::LicenseStateException)
        {
            ERROR << "Current license is not valid" << std::endl;
            if (!license->isActive())
            {
                ERROR << "License is inactive" << std::endl;
                return false;
            }
            if (license->isExpired())
            {
                ERROR << "License is expired" << std::endl;
                return false;
            }
            if (!license->isEnabled())
            {
                ERROR << "License is disabled" << std::endl;
                return false;
            }
        }
        catch (std::exception &e)
        {
            ERROR << e.what();
            return false;
        }
    }
    else
    {
        ERROR << "No local license found";
        return false;
    }

    auto devVars = license->getDeviceVariables();
    std::string licenseRole = std::find_if(devVars.begin(), devVars.end(),
                                           [](LicenseSpring::DeviceVariable &item)
                                           {
                                               return item.name() == "LicenseRole";
                                           })->value();

    NodeRole nodeRoleEnum = std::find_if(string2noderole.begin(),
                                         string2noderole.end(),
                                         [&licenseRole](std::pair<std::string, NodeRole> &item)
                                         {
                                             return item.first == licenseRole;
                                         })->second;

    /*
    if (m_license.licenseNodeRole != nodeRoleEnum)
    {
        ERROR << "License node role did not match product node role!";
        return false;
    }
    */

    return true;
}
#endif

// ---------------------------------------------------------------------

} // namespace uh::licensing
