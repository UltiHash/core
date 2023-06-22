//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <licensing/backend.h>
#include <io/temp_file.h>

#include <filesystem>
#include <utility>
#include <map>

#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/LicenseID.h>
#include <LicenseSpring/LicenseFileStorage.h>
#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/Exceptions.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace uh::licensing
{

struct api_config
{
    const std::string productId;
};

struct credential_config
{
    const std::string appName;
    const std::string appVersion;
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
    std::string key;
};

struct license_config
{
    const NodeRole licenseNodeRole = NodeRole::DataNode;
    std::filesystem::path license_path = "/var/lib";
};

// ---------------------------------------------------------------------

class check_airgap_license : public backend
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

    // without activation
    explicit check_airgap_license(uh::licensing::license_config license_config,
                                  uh::licensing::api_config apiKey_input,
                                  uh::licensing::credential_config credentialConfig_input);

    /**
     * the default license is not timed
     *
     * @param license_path is the path to the license file
     * @return if the license file is valid for the implemented service role and features
     */
    bool valid() override;

    /**
     * Return true if the given feature is enabled.
     */
    bool has_feature(feature f) const override;

    /**
     * Return a feature parameter as an string.
     */
    std::string feature_arg_string(feature f, const std::string& name) const override;

    /**
     * Return a feature parameter as an string.
     */
    std::size_t feature_arg_size_t(feature f, const std::string& name) const override;

private:
    void reload(LicenseSpring::License& license);

    LicenseSpring::LicenseID m_id;
    std::shared_ptr<LicenseSpring::Configuration> m_config;
    std::shared_ptr<LicenseSpring::LicenseFileStorageBase> m_storage;
    std::shared_ptr<LicenseSpring::LicenseManager> m_manager;
    std::shared_ptr<LicenseSpring::License> m_license;

    std::map<feature, boost::property_tree::ptree> m_features;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
