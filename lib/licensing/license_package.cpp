//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/license_package.h"

#include <utility>

namespace uh::licensing
{

license_package::license_package(std::shared_ptr<uh::licensing::check_airgap_license> check_license)
    :
    m_check_license(std::move(check_license))
{
    if (m_check_license->valid())feature_activation();
}

// ---------------------------------------------------------------------

bool license_package::feature_enabled(feature f) const
{
    return m_features.at(f);
}

// ---------------------------------------------------------------------

void license_package::check(license_package::metered_feature smf, std::size_t alloc)
{
    if (!has_metred_feature(smf))
    THROW(util::exception, "Soft metered feature not found!");

    if (m_soft_metered_features.at(smf)->stored_val + alloc <= m_soft_metered_features.at(smf)->soft_limit_val)
    {
        m_soft_metered_features.at(smf)->stored_val += alloc;
        m_soft_metered_features.at(smf)->warn_once = true;
        return;
    }

    if (m_soft_metered_features.at(smf)->stored_val + alloc <= m_soft_metered_features.at(smf)->hard_limit_val)
    {
        m_soft_metered_features.at(smf)->stored_val += alloc;
        if (m_soft_metered_features.at(smf)->warn_once)
        {
            WARNING
                << "Soft metered storage resource reached warning limit with "
                    + std::to_string(m_soft_metered_features.at(smf)->stored_val)
                    + " of a maximum of "
                    + std::to_string(m_soft_metered_features.at(smf)->hard_limit_val) + " bytes.";
            m_soft_metered_features.at(smf)->warn_once = false;
        }

        return;
    }

    THROW(util::exception, "Out of licensed storage!");
}

// ---------------------------------------------------------------------

void
license_package::add_metred_feature(license_package::metered_feature smf,
                                    const std::shared_ptr<metered_resource> &smr)
{
    m_soft_metered_features.emplace(smf, smr);
}

// ---------------------------------------------------------------------

bool license_package::has_metred_feature(license_package::metered_feature smf)
{
    return m_soft_metered_features.contains(smf);
}

// ---------------------------------------------------------------------

void license_package::feature_activation()
{
    auto feature_online = m_check_license->getCustomAndFeatureFields();

    if (feature_online.contains(WARN_STORAGE_STRING))
    {
        if (!feature_online.contains(LIMIT_STORAGE_STRING))
        {
            THROW(util::exception, "There was a misconfiguration due to the storage limits of your license. "
                                   "Please contact customer support!");
        }

        try
        {
            uint64_t warnLevel = std::stoull(feature_online.at(WARN_STORAGE_STRING));
            uint64_t limitLevel = std::stoull(feature_online.at(LIMIT_STORAGE_STRING));

            add_metred_feature(
                uh::licensing::license_package::metered_feature::LIMIT_STORAGE_CAPACITY,
                std::make_shared<metered_resource>(limitLevel, warnLevel)
            );

            feature_online.erase(WARN_STORAGE_STRING);
            feature_online.erase(LIMIT_STORAGE_STRING);
        }
        catch (std::exception &e)
        {
            THROW(util::exception, "Parsing license specific soft storage limit failed for this reason: " +
                std::string(e.what()) + "\n Please contact customer support to correct "
                                        "your license!");
        }
    }
    else
    {
        if (feature_online.contains(LIMIT_STORAGE_STRING))
        {
            try
            {
                uint64_t limitLevel = std::stoull(feature_online.at(LIMIT_STORAGE_STRING));

                add_metred_feature(
                    uh::licensing::license_package::metered_feature::LIMIT_STORAGE_CAPACITY,
                    std::make_shared<uh::licensing::metered_resource>(limitLevel,
                                                                                   limitLevel)
                );
            }
            catch (std::exception &e)
            {
                THROW(util::exception, "Parsing license specific hard storage limit failed for this reason: " +
                    std::string(e.what()) + "\n Please contact customer support to correct "
                                            "your license!");
            }
        }
    }

    m_features.emplace(feature::DEDUPLICATION, feature_online.contains(DEDUPLICATION_STRING));
    m_features.emplace(feature::METRICS, feature_online.contains(METRICS_STRING));

}

// ---------------------------------------------------------------------

bool license_package::valid()
{
    return m_check_license->valid();
}

// ---------------------------------------------------------------------

} // namespace uh::licensing