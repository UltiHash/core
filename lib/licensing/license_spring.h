//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <licensing/backend.h>

#include <filesystem>
#include <map>

#include <boost/property_tree/ptree.hpp>

#include <LicenseSpring/LicenseManager.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

struct license_spring_config
{
    std::string productId;
    std::string appName;
    std::string appVersion;
    std::filesystem::path path;
};

// ---------------------------------------------------------------------

class license_spring : public backend
{
public:
    /**
     * Construct and activate license.
     */
    explicit license_spring(const license_spring_config& config,
                            const std::string& key);

    // without activation
    explicit license_spring(const license_spring_config& config);

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

    std::shared_ptr<LicenseSpring::LicenseManager> m_manager;
    std::shared_ptr<LicenseSpring::License> m_license;

    std::map<feature, boost::property_tree::ptree> m_features;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
