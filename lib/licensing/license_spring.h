
#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <licensing/common.h>
#include <licensing/backend.h>

#ifdef USE_LICENSE_SPRING

#include <filesystem>
#include <map>

#include <boost/property_tree/ptree.hpp>

#include <LicenseSpring/LicenseManager.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

class license_spring : public backend
{
public:
    /**
     * Construct and activate license.
     */
    explicit license_spring(const license_config& config,
                            const std::string& key);

    // without activation
    explicit license_spring(const license_config& config);

    /**
     * Return true if the given feature is enabled.
     */
    [[nodiscard]] bool has_feature(feature f) const override;

    /**
     * Return a feature parameter as an string.
     */
    [[nodiscard]] std::string feature_arg_string(feature f, const std::string& name) const override;

    /**
     * Return a feature parameter as an string.
     */
    [[nodiscard]] std::size_t feature_arg_size_t(feature f, const std::string& name) const override;

private:
    void reload();

    std::shared_ptr<LicenseSpring::LicenseManager> m_manager;
    std::shared_ptr<LicenseSpring::License> m_license;

    std::map<feature, boost::property_tree::ptree> m_features;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing
#endif
#endif //CORE_CHECK_LICENSE_H
