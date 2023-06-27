#include <licensing/demo_license.h>

#include <util/exception.h>

namespace uh::licensing
{

demo_license::demo_license()
{
    reload();
}

// ---------------------------------------------------------------------

bool demo_license::has_feature(feature f) const
{
    return m_features.contains(f);
}

// ---------------------------------------------------------------------

std::string demo_license::feature_arg_string(feature f, const std::string& name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end()) {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::string>(name);
}

// ---------------------------------------------------------------------

std::size_t demo_license::feature_arg_size_t(feature f, const std::string& name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end()) {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::size_t>(name);
}

// ---------------------------------------------------------------------

void demo_license::reload()
{
    std::map<feature, boost::property_tree::ptree> features;

    const auto feat = feature::STORAGE;
    auto &ptree = features[feat];

    const uint64_t one_GB_demo = 1024 * 1024 * 1024;

    auto md = "{\"max\":" + std::to_string(one_GB_demo * 10) + ",\"max_soft\":"
        + std::to_string(one_GB_demo * 9) + ",\"min\":0}";
    std::stringstream metadata(md);
    boost::property_tree::read_json(metadata, ptree);

    std::swap(features, m_features);
}

} // licensing