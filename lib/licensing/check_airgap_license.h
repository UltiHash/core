//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <licensing/backend.h>

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

// ---------------------------------------------------------------------

struct ls_airgap_config
{
    const std::string productId;
    const std::string appName;
    const std::string appVersion;
    std::filesystem::path path;
};

// ---------------------------------------------------------------------

class check_airgap_license : public backend
{
public:

    /**
     * Construct and activate license.
     */
    explicit check_airgap_license(const ls_airgap_config& config,
                                  const std::string& key);

    // without activation
    explicit check_airgap_license(const ls_airgap_config& config);

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
    void reload();

    std::shared_ptr<LicenseSpring::Configuration> m_config;
    std::shared_ptr<LicenseSpring::LicenseFileStorageBase> m_storage;
    std::shared_ptr<LicenseSpring::LicenseManager> m_manager;
    std::shared_ptr<LicenseSpring::License> m_license;

    std::map<feature, boost::property_tree::ptree> m_features;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
