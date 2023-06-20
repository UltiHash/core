//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/license_package.h"

#include <utility>
#include "logging/logging_boost.h"

#include "soft_metered_storage_resource.h"

namespace uh::licensing
{

license_package::license_package(uh::licensing::license_config license_config,
                                 uh::licensing::api_config apiKey_input,
                                 uh::licensing::credential_config credentialConfig_input,
                                 uh::licensing::license_activate_config license_activate_input):
    check_license(
        std::move(license_config),
        std::move(apiKey_input),
        std::move(credentialConfig_input),
        std::move(license_activate_input))
{
    feature_activation();
}

// ---------------------------------------------------------------------

bool license_package::check_feature_enabled(license_package::feature f) const
{
    return features.at(f);
}

// ---------------------------------------------------------------------

bool license_package::hard_limit_allocate(license_package::hard_metered_feature hmf, std::size_t alloc)
{
    if (!hard_metered_features.contains(hmf))
    THROW(util::exception, "Hard metred feature was not avialable!");

    return hard_metered_features.at(hmf)->hard_limit_allocate(alloc);
}

// ---------------------------------------------------------------------

bool license_package::soft_limit_allocate(license_package::soft_metered_feature smf, std::size_t alloc)
{
    if (!soft_metered_features.contains(smf))
    THROW(util::exception, "Soft metred feature was not avialable!");

    return soft_metered_features.at(smf)->soft_limit_allocate(alloc);
}

// ---------------------------------------------------------------------

void license_package::deallocate(license_package::hard_metered_feature hmf, std::size_t dealloc)
{
    if (soft_metered_features.contains(static_cast<const soft_metered_feature>(hmf)))
    {
        soft_metered_features.at(static_cast<const soft_metered_feature>(hmf))->deallocate(dealloc);
    }
    else
    {
        if (!hard_metered_features.contains(hmf))
            return;

        hard_metered_features.at(hmf)->deallocate(dealloc);
    }
}

// ---------------------------------------------------------------------

std::size_t license_package::free_count(license_package::hard_metered_feature hmf)
{
    if (soft_metered_features.contains(static_cast<const soft_metered_feature>(hmf)))
    {
        return soft_metered_features.at(static_cast<const soft_metered_feature>(hmf))->free_count();
    }
    else
    {
        if (!hard_metered_features.contains(hmf))
            return {};

        return hard_metered_features.at(hmf)->free_count();
    }
}

// ---------------------------------------------------------------------

void license_package::add_hard_metred_feature(license_package::hard_metered_feature mf, metred_resource *mr)
{
    hard_metered_features.emplace(mf, mr);
}

// ---------------------------------------------------------------------

void
license_package::add_soft_metred_feature(license_package::soft_metered_feature smf, soft_metered_resource *smr)
{
    hard_metered_features.emplace(static_cast<hard_metered_feature>(smf), smr);
    soft_metered_features.emplace(smf, smr);
}

// ---------------------------------------------------------------------

bool license_package::has_hard_metred_feature(license_package::hard_metered_feature mf)
{
    return hard_metered_features.contains(mf);
}

// ---------------------------------------------------------------------

bool license_package::has_soft_metred_feature(license_package::soft_metered_feature smf)
{
    return soft_metered_features.contains(smf);
}

// ---------------------------------------------------------------------

void license_package::feature_activation()
{
    auto feature_online = getCustomAndFeatureFields();

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

            add_soft_metred_feature(
                uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                new soft_metered_storage_resource(limitLevel, warnLevel)
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

                add_hard_metred_feature(
                    uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY,
                    new uh::licensing::soft_metered_storage_resource(limitLevel, limitLevel)
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

    features.emplace(feature::DEDUPLICATION, feature_online.contains(DEDUPLICATION_STRING));
    features.emplace(feature::METRICS, feature_online.contains(METRICS_STRING));

}

// ---------------------------------------------------------------------

} // namespace uh::licensing