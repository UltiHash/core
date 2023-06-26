#ifndef DEMO_LICENSE_H
#define DEMO_LICENSE_H

#include <licensing/backend.h>
#include <licensing/license_spring.h>

namespace uh::licensing
{

class demo_license: public backend
{
public:
    /**
     * Construct and activate license.
     */
    explicit demo_license(const license_spring_config& config,
                            const std::string& key);

    // without activation
    explicit demo_license(const license_spring_config& config);

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

    std::map<feature, boost::property_tree::ptree> m_features;
};

} // licensing

#endif //DEMO_LICENSE_H
