//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_airgap_license.h"
#include "util/exception.h"
#include "logging/logging_boost.h"
#include "io/temp_file.h"
#include "io/sha512.h"

#include "LicenseSpring/Exceptions.h"

#include <string>
#include <iostream>
#include <utility>

#include "boost/property_tree/ptree.hpp"
#include "boost/algorithm/string.hpp"

namespace uh::licensing
{

// ---------------------------------------------------------------------

check_airgap_license::check_airgap_license(uh::licensing::license_config license_config,
                                           uh::licensing::api_config apiKey_input,
                                           uh::licensing::credential_config credentialConfig_input,
                                           uh::licensing::license_activate_config license_activate_input)
    :
    m_license(std::move(license_config)),
    m_api(std::move(apiKey_input)),
    m_credential(std::move(credentialConfig_input)),
    m_license_activate(std::move(license_activate_input))
{
    if(std::filesystem::is_directory(m_license.license_path))
        m_license.license_path /= m_credential.appName + ".lic";
}

// ---------------------------------------------------------------------

bool check_airgap_license::valid()
{
    if (!getLicenseActivateConfig().key.empty())
    {
        return licenseRegister(LicenseSpring::LicenseID::fromKey(getLicenseActivateConfig().key));
    }
    else
    {
        return licenseRegister(LicenseSpring::LicenseID::fromUser(getLicenseActivateConfig().username,
                                                                  getLicenseActivateConfig().password));
    }
}

// ---------------------------------------------------------------------

bool check_airgap_license::exists() const
{
    return std::filesystem::exists(m_license.license_path) and std::filesystem::is_regular_file(m_license.license_path);
}

// ---------------------------------------------------------------------

std::map<std::string, std::string>
check_airgap_license::getCustomAndFeatureFields()
{
    auto feature_item_registry = std::vector<std::string>({"limitStorage", "warnStorage"});

    auto licenseFileStorage =
        std::make_shared<LicenseSpring::FileStorageWithLock>(
            LicenseSpring::FileStorageWithLock(m_license.license_path.wstring()));

    auto licenseManager =
        LicenseSpring::LicenseManager::create(getLicenseSpringConfig(), licenseFileStorage);

    LicenseSpring::License::ptr_t license = licenseManager->reloadLicense();
    auto cf = license->customFields();
    auto features = license->features();

    std::map<std::string, std::string> out_map;

    for (const auto &item : cf)
    {
        out_map.emplace(item.fieldName(), item.fieldValue());
    }

    for (const auto &item : features)
    {
        std::string metadata = item.metadata();
        boost::algorithm::replace_all(metadata, "\\", "");

        if (!metadata.empty())
        {
            metadata = metadata.substr(1, metadata.size() - 2);
            std::stringstream meta_ss(metadata);
            boost::property_tree::ptree pt;
            boost::property_tree::read_json(meta_ss, pt);

            for (const auto &item2 : feature_item_registry)
            {
                auto optional_child = pt.get_child_optional(item2);

                if (optional_child)
                {
                    auto read_entry = optional_child->get_value<std::string>();
                    out_map.emplace(item2, read_entry);
                }
            }
        }
        else
        {
            out_map.emplace(item.name(), item.metadata());
        }
    }

    return out_map;
}

// ---------------------------------------------------------------------

bool check_airgap_license::licenseRegister(const LicenseSpring::LicenseID &licenseId)
{
    auto licenseFileStorage = std::make_shared<LicenseSpring::FileStorageWithLock>
        (m_license.license_path.wstring());

    if (!std::filesystem::exists(m_license.license_path))
        licenseFileStorage->create(m_license.license_path.wstring());

    auto licenseManager =
        LicenseSpring::LicenseManager::create(getLicenseSpringConfig(), licenseFileStorage);

    LicenseSpring::License::ptr_t license = licenseManager->reloadLicense();

    if (!license_check(license))
    {
        try
        {
            license = licenseManager->activateLicense(licenseId);

            std::string role_set_string, license_type_set_string;

            switch (m_license.licenseNodeRole)
            {
                case NodeRole::AgencyNode:role_set_string = noderole2string[NodeRole::AgencyNode];
                    break;
                case NodeRole::DataNode:role_set_string = noderole2string[NodeRole::DataNode];
                    break;
                default:THROW(util::exception, "No license role detected!");
            }

            switch (m_license.licenseTypeInternal)
            {
                case LicenseTypeEnum::AirgapKeyOnline:
                    license_type_set_string = licensetype2string[LicenseTypeEnum::AirgapKeyOnline];
                    license->addDeviceVariable("Key", licenseId.key());
                    break;
                default:THROW(util::exception, "No license type detected!");
            }

            license->addDeviceVariable("LicenseRole", role_set_string);
            license->addDeviceVariable("LicenseType", license_type_set_string);
            INFO << "License status: " << license->status();
        }
        catch (std::exception &e)
        {
            ERROR << e.what();
            return false;
        }
    }
    else return true;

    if (license != nullptr)
    {
        return license_check(license);
    }
    else return false;
}

// ---------------------------------------------------------------------

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

    if (getLicense().licenseNodeRole != nodeRoleEnum)
    {
        ERROR << "License node role did not match product node role!";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------

LicenseSpring::ExtendedOptions check_airgap_license::getOptions()
{
    //Collecting network info
    LicenseSpring::ExtendedOptions options;

    auto lic = getLicense();

    options.collectNetworkInfo(lic.collectNetworkInfo);
    options.enableLogging(lic.enableLogging);
    options.enableVMDetection(lic.enableVMDetection);
    options.enableSSLCheck(lic.enableSSLcheck);
    options.enableGuardFile(lic.enableGuardFile);

    return options;
}

// ---------------------------------------------------------------------

const license_config &check_airgap_license::getLicense() const
{
    return m_license;
}

// ---------------------------------------------------------------------

const api_config &check_airgap_license::getApi() const
{
    return m_api;
}

// ---------------------------------------------------------------------

const credential_config &check_airgap_license::getCredentials() const
{
    return m_credential;
}

// ---------------------------------------------------------------------

std::shared_ptr<LicenseSpring::Configuration> check_airgap_license::getLicenseSpringConfig()
{
    LicenseSpring::ExtendedOptions options = getOptions();
    uh::licensing::api_config api = getApi();
    uh::licensing::credential_config credentials = getCredentials();

    std::shared_ptr<LicenseSpring::Configuration> pConfiguration = LicenseSpring::Configuration::Create(
        api.apiKey, // your LicenseSpring API key (UUID)
        api.sharedKey, // your LicenseSpring Shared key
        api.productId, // product code that you specified in LicenseSpring for your application
        credentials.appName,
        credentials.appVersion,
        options);

    return pConfiguration;
}

// ---------------------------------------------------------------------

license_activate_config check_airgap_license::getLicenseActivateConfig()
{
    auto licenseFileStorage =
        std::make_shared<LicenseSpring::FileStorageWithLock>(LicenseSpring::
                                                             FileStorageWithLock(m_license.license_path.wstring()));

    if (!std::filesystem::exists(m_license.license_path))
        licenseFileStorage->create(m_license.license_path.wstring());

    auto licenseManager =
        LicenseSpring::LicenseManager::create(getLicenseSpringConfig(), licenseFileStorage);

    LicenseSpring::License::ptr_t license = licenseManager->reloadLicense();

    if(license == nullptr)
        return m_license_activate;

    auto deviceVars = license->getDeviceVariables();

    auto valueFinder = [&deviceVars](std::string_view input)
    {
        return std::string(
            std::find_if(deviceVars.begin(), deviceVars.end(),
                         [&input](LicenseSpring::DeviceVariable &item)
                         {
                             return item.name() == input;
                         })->value());
    };

    const std::string_view licenseType_indicator = "LicenseType";

    //detect license type
    std::string licType_string = valueFinder(licenseType_indicator);
    LicenseTypeEnum licType_enum = std::find_if(string2licensetype.begin(), string2licensetype.end(),
                                                [&licType_string](auto &item)
                                                {
                                                    return item.first == licType_string;
                                                })->second;

    if (licType_enum == LicenseTypeEnum::AirgapKeyOnline)
    {
        m_license_activate = license_activate_config{ .key = valueFinder("Key") };
        return m_license_activate;
    }
    else
    {
        m_license_activate = license_activate_config{ .username = valueFinder("Username"),
                                                      .password = valueFinder("Password") };
        return m_license_activate;
    }

}

} // namespace uh::licensing
