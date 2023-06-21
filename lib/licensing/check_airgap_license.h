//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <filesystem>
#include <utility>
#include <map>

#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/LicenseID.h>
#include <LicenseSpring/LicenseFileStorage.h>
#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/Exceptions.h>

#include <io/temp_file.h>

#include "boost/property_tree/json_parser.hpp"

namespace uh::licensing
{

struct api_config
{
public:

    const std::string apiKey;
    const std::string sharedKey;
    const std::string productId;

    /**
     * License Spring info and functions
     *
     * apiKey_crypt is 36 characters long
     * sharedKey_crypt is 43 characters long
     * productId_crypt is 6 characters long
     */

    api_config(std::string apiKey_input, std::string shardKey_input, std::string productId_input)
        :
        apiKey(std::move(apiKey_input)),
        sharedKey(std::move(shardKey_input)),
        productId(std::move(productId_input))
    {}
};

struct credential_config
{
public:

    const std::string appName;
    const std::string appVersion;

    credential_config(std::string appName_input, std::string appVersion_input)
        :
        appName(std::move(appName_input)),
        appVersion(std::move(appVersion_input))
    {}

    bool empty()
    {
        return appName.empty() and appVersion.empty();
    }
};

// ---------------------------------------------------------------------

enum class LicenseTypeEnum
{
    AirgapKeyOnline,
    OtherLicense
};

static std::vector<std::pair<std::string, LicenseTypeEnum>> string2licensetype = {
    {"AirgapKeyOnline", LicenseTypeEnum::AirgapKeyOnline},
    {"OtherLicense", LicenseTypeEnum::OtherLicense}
};

static std::unordered_map<LicenseTypeEnum, std::string> licensetype2string = {
    {LicenseTypeEnum::AirgapKeyOnline, "AirgapKeyOnline"},
    {LicenseTypeEnum::OtherLicense, "OtherLicense"}
};

// ---------------------------------------------------------------------

enum class NodeRole: unsigned char
{
    DataNode,
    AgencyNode,
    OtherRole
};

static std::vector<std::pair<std::string, NodeRole>> string2noderole = {
    {"uh-data-node", NodeRole::DataNode},
    {"uh-agency-node", NodeRole::AgencyNode},
    {"uh-other-node", NodeRole::OtherRole}
};

static std::unordered_map<NodeRole, std::string> noderole2string = {
    {NodeRole::DataNode, "uh-data-node"},
    {NodeRole::AgencyNode, "uh-agency-node"},
    {NodeRole::OtherRole, "uh-other-node"}
};

struct license_activate_config
{
public:
    std::string username;
    std::string password;
    std::string key;

    license_activate_config(std::string username_input, std::string password_input)
        :
        username(std::move(username_input)),
        password(std::move(password_input))
    {}

    explicit license_activate_config(std::string key_input)
        :
        key(std::move(key_input))
    {}
};

struct license_config
{
public:

    const LicenseTypeEnum licenseTypeInternal = LicenseTypeEnum::AirgapKeyOnline;
    const NodeRole licenseNodeRole = NodeRole::DataNode;

    const bool collectNetworkInfo = true;
    const bool enableLogging = true;
    const bool enableVMDetection = false;
    const bool enableSSLcheck = true;
    const bool enableGuardFile = true;

    std::filesystem::path license_path = "/var/lib";
    bool replace_license = false;

    license_config() = default;

    license_config(const LicenseTypeEnum licenseType,
                   const NodeRole nodeRole,
                   const bool networkInfo_active,
                   const bool logging_active,
                   const bool vm_detect_active,
                   const bool ssl_active,
                   const bool guard_file_active,
                   std::filesystem::path license_path_input,
                   bool replace_license)
        :
        licenseTypeInternal(licenseType),
        licenseNodeRole(nodeRole),
        collectNetworkInfo(networkInfo_active),
        enableLogging(logging_active),
        enableVMDetection(vm_detect_active),
        enableSSLcheck(ssl_active),
        enableGuardFile(guard_file_active),
        license_path(std::move(license_path_input)),
        replace_license(replace_license)
    {}
};

// ---------------------------------------------------------------------

class check_airgap_license
{

public:

    /**
     * manages license file creation and parsing
     * First line license: license role
     * Second line license:
     *
     * @param license_directory is the path where a license file is stored
     */
    explicit check_airgap_license(uh::licensing::license_config license_config,
                                  uh::licensing::api_config apiKey_input,
                                  uh::licensing::credential_config credentialConfig_input,
                                  uh::licensing::license_activate_config license_activate_input);

    /**
     * the default license is not timed
     *
     * @param license_path is the path to the license file
     * @return if the license file is valid for the implemented service role and features
     */
    bool valid();

    /**
     *
     * @return if license file representation is written to disk
     */
    [[nodiscard]] bool exists() const;

    /**
     *
     * @return a map of key value pairs that will configure the application
     */
    [[nodiscard]] std::map<std::string, std::string>
    getCustomAndFeatureFields();

protected:
    api_config m_api;
    credential_config m_credential;
    license_config m_license;

    /**
     *
     * @param licenseManager parsed license file and API authentication
     * @param licenseId license sign method either user based or key based
     * @return success or failure of license activation and check
     */
    bool licenseRegister(const LicenseSpring::LicenseID &licenseId);

    /**
     *
     * @param license is the incoming parsed license file
     * @return if the license was still valid
     */
    bool license_check(const LicenseSpring::License::ptr_t &license);

    LicenseSpring::ExtendedOptions getOptions();

    [[nodiscard]] const license_config &getLicense() const;

    [[nodiscard]] const api_config &getApi() const;

    [[nodiscard]] const credential_config &getCredentials() const;

    std::shared_ptr<LicenseSpring::Configuration> getLicenseSpringConfig();

    license_activate_config getLicenseActivateConfig();

private:
    uh::licensing::license_activate_config m_license_activate;
};

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
