#ifndef DEMO_LICENSE_H
#define DEMO_LICENSE_H

#include <licensing/backend.h>

#include <map>

#include <boost/property_tree/json_parser.hpp>

namespace uh::licensing
{

class demo_license: public backend
{
public:
    /**
     * Construct and activate license.
     */
    demo_license();

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
