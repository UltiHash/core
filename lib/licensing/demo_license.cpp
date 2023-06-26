//
// Created by benjamin-elias on 26.06.23.
//

#include <licensing/demo_license.h>
#include <license_spring_credentials.h>

#include <util/exception.h>

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

}

} // licensing