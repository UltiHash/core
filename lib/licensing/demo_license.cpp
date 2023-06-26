#include <licensing/demo_license.h>

#include <util/exception.h>

#include <boost/property_tree/json_parser.hpp>

namespace uh::licensing
{

demo_license::demo_license(const license_spring_config &config, const std::string &key)
{
    reload();
}

// ---------------------------------------------------------------------

demo_license::demo_license(const license_spring_config &config)
{
    reload();
}

// ---------------------------------------------------------------------

bool demo_license::valid()
{
    return false;
}

// ---------------------------------------------------------------------

bool demo_license::has_feature(feature f) const
{
    return m_features.contains(f);
}

// ---------------------------------------------------------------------

std::string demo_license::feature_arg_string(feature f, const std::string &name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end())
    {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::string>(name);
}

// ---------------------------------------------------------------------

std::size_t demo_license::feature_arg_size_t(feature f, const std::string &name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end())
    {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::size_t>(name);
}

// ---------------------------------------------------------------------

void demo_license::reload()
{
    std::map<feature, boost::property_tree::ptree> features;

    const auto feat = feature::STORAGE;
    auto& ptree = features[feat];

    auto md = "{\"max\":10737418240,\"max_soft\":10200547328,\"min\":0}";
    std::stringstream metadata(md);
    boost::property_tree::read_json(metadata, ptree);

    std::swap(features, m_features);
}

} // licensing