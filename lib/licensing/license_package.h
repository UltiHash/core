//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSE_PACKAGE_H
#define CORE_LICENSE_PACKAGE_H

#include "licensing/metred_resource.h"
#include "licensing/soft_metered_resource.h"
#include "licensing/check_airgap_license.h"
#include "util/exception.h"
#include "logging/logging_boost.h"

#include <map>
#include <set>
#include <filesystem>
#include <algorithm>

namespace uh::licensing
{

class license_package: check_airgap_license
{

public:

    /*
     * Licensing server feature flags
     */

    const std::string METRICS_STRING = "Metrics";
    const std::string DEDUPLICATION_STRING = "Deduplication";
    const std::string WARN_STORAGE_STRING = "warnStorage";
    const std::string LIMIT_STORAGE_STRING = "limitStorage";

    /**
     *
     * Check if `feature`is enabled in the configured license.
     * Block feature if no license support is detected
     */

    enum class feature: unsigned char
    {
        METRICS,
        DEDUPLICATION
    };

    /**
     * manages features and metered setup on top of the license checker
     *
     * @param config license file input
     * @throws if license is invalid or cannot be loaded
     */
    explicit license_package(uh::licensing::license_config license_config,
                             uh::licensing::api_config apiKey_input,
                             uh::licensing::credential_config credentialConfig_input,
                             uh::licensing::license_activate_config license_activate_input);

    /**
     *
     * Check if `hard_metered_feature is set in the configured license.
     * Limit the resource capacity to the value set by license.
     * Warn if resource boundary is close to be hit.
     * @throw if resource boundary will be exceeded by resource allocation
     */
    enum class hard_metered_feature: unsigned char
    {
        LIMIT_CPU_COUNT = 0,
        LIMIT_STORAGE_CAPACITY = 1,
        LIMIT_NETWORK_CONNECTIONS = 2
    };

    /**
     *
     * Warn if resource boundary is close to be hit.
     * @throw if resource boundary will be exceeded by resource allocation
     */
    enum class soft_metered_feature: unsigned char
    {
        LIMIT_STORAGE_CAPACITY = 1,
        LIMIT_NETWORK_CONNECTIONS = 2
    };

    /**
     *
     * @param f check feature on or off/true or false
     * @throw feature state not specified by license and by that not listed
     * @return feature state
     */
    [[nodiscard]] bool check_feature_enabled(feature f) const;

    /**
     * first try soft limit allocate, if successful, hard limit allocate will also be set
     * HINT: a not contained limit is no limit
     *
     * @param hmf hard metred feature that should check for available allocation
     * @param alloc some resource
     * @return if allocation was successful and metered counter updated --> valid operation
     */
    bool hard_limit_allocate(hard_metered_feature hmf, std::size_t alloc);

    /**
     * first try soft limit allocate, if successful, hard limit allocate will also be set
     * HINT: a not contained limit is no limit
     *
     * @param smf soft metred feature that should check for available allocation without warning
     * @param alloc some resource
     * @return if allocation was successful and metered counter updated without warning required
     */
    bool soft_limit_allocate(soft_metered_feature smf, std::size_t alloc);

    /**
     *
     * @param hmf hard metred feature to be deallocated
     * @param dealloc resources to be registered deallocated
     */
    void deallocate(hard_metered_feature hmf, std::size_t dealloc);

    /**
     *
     * @param hmf metred feature to test free count on
     * @return number of free resources of metred resource
     */
    std::size_t free_count(hard_metered_feature hmf);

    /**
     *
     * @param mf metered feature to be registered
     * @param mr metered resource class to be checked repeatedly
     */
    void add_hard_metred_feature(hard_metered_feature mf, metred_resource *mr);

    /**
     *
     * @param smf soft metered feature to be registered
     * @param smr soft metered resource class to be checked repeatedly
     */
    void add_soft_metred_feature(soft_metered_feature smf, soft_metered_resource *smr);

    /**
     *
     * @param mf metered feature to check
     * @return if hard metred feature is available
     */
    bool has_hard_metred_feature(hard_metered_feature mf);

    /**
     *
     * @param smf metered feature to check
     * @return if soft metred feature is available
     */
    bool has_soft_metred_feature(license_package::soft_metered_feature smf);

private:

    std::map<feature, bool> features;
    std::map<hard_metered_feature, metred_resource *> hard_metered_features;
    std::map<soft_metered_feature, soft_metered_resource *> soft_metered_features;

    /**
     * activates online defined features and limits
     */
    void feature_activation();
};

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
