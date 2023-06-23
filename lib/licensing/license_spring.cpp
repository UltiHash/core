//
// Created by benjamin-elias on 06.06.23.
//

#include <licensing/license_spring.h>
#include <license_spring_credentials.h>

#include <util/exception.h>

#include <boost/property_tree/json_parser.hpp>

#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/LicenseFileStorage.h>
#include <LicenseSpring/LicenseID.h>


using namespace LicenseSpring;

namespace uh::licensing
{

namespace
{

// ---------------------------------------------------------------------

std::shared_ptr<LicenseSpring::LicenseManager> mk_manager(const license_spring_config& config)
{
    ExtendedOptions options;

#ifdef DEBUG
    options.enableLogging(true);
#endif
    options.enableSSLCheck(true);

    auto cfg = Configuration::Create(
        EncryptStr(LICENSE_SPRING_API_KEY),
        EncryptStr(LICENSE_SPRING_SHARED_KEY),
        config.productId,
        config.appName,
        config.appVersion,
        options);

    auto storage = LicenseFileStorage::create(config.path.wstring());
    return LicenseManager::create(cfg, storage);
}

// ---------------------------------------------------------------------

}

// ---------------------------------------------------------------------

license_spring::license_spring(const license_spring_config& config,
                               const std::string& key)
    : m_manager(mk_manager(config)),
      m_license(m_manager->activateLicense(LicenseID::fromKey(key)))
{
    m_license->check();
    reload();
}

// ---------------------------------------------------------------------

license_spring::license_spring(const license_spring_config& config)
    : m_manager(mk_manager(config)),
      m_license(m_manager->reloadLicense())
{
    reload();
}

// ---------------------------------------------------------------------

bool license_spring::valid()
{
    try
    {
        m_license->localCheck();
    }
    catch (...)
    {
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------

bool license_spring::has_feature(feature f) const
{
    return m_features.contains(f);
}

// ---------------------------------------------------------------------

std::string license_spring::feature_arg_string(feature f, const std::string& name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end())
    {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::string>(name);
}

// ---------------------------------------------------------------------

std::size_t license_spring::feature_arg_size_t(feature f, const std::string& name) const
{
    auto it = m_features.find(f);
    if (it == m_features.end())
    {
        THROW(util::exception, "feature argument not defined: " + name);
    }

    return it->second.get<std::size_t>(name);
}

// ---------------------------------------------------------------------

void license_spring::reload()
{
    if (!m_license)
    {
        THROW(util::exception, "could not load license");
    }

    m_license->localCheck();

    if (!m_license->isActive())
    {
        THROW(util::exception, "license is not activated");
    }

    if (m_license->isExpired())
    {
        THROW(util::exception, "license is expired");
    }

    if (!m_license->isEnabled())
    {
        THROW(util::exception, "license is not enabled");
    }

    std::map<feature, boost::property_tree::ptree> features;

    for (const auto& feature : m_license->features())
    {
        try
        {
            auto feat = feature_from_string(feature.name());
            auto& ptree = features[feat];

            auto md = feature.metadata();
            if (!md.empty())
            {
                std::stringstream metadata(md);
                boost::property_tree::read_json(metadata, ptree);
            }
        }
        catch (const util::exception& e)
        {
            continue;
        }
    }

    std::swap(features, m_features);
}

// ---------------------------------------------------------------------

} // namespace uh::licensing
