//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSE_PACKAGE_H
#define CORE_LICENSE_PACKAGE_H

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

class license_package
{

public:

    /*
     * Licensing server feature flags
     */

    const std::string METRICS_STRING = "Metrics";
    const std::string DEDUPLICATION_STRING = "Deduplication";

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
    explicit license_package(std::shared_ptr<uh::licensing::check_airgap_license> check_license);

    /**
     *
     * Warn if resource boundary is close to be hit.
     * @throw if resource boundary will be exceeded by resource allocation
     */
    enum class soft_metered_feature: unsigned char
    {
        LIMIT_CPU_COUNT = 0,
        LIMIT_STORAGE_CAPACITY = 1,
        LIMIT_NETWORK_CONNECTIONS = 2
    };

    /**
     *
     * @param f check feature on or off/true or false
     * @throw feature state not specified by license and by that not listed
     * @return feature state
     */
    [[nodiscard]] bool feature_enabled(feature f) const;

    /**
     * first try soft limit allocate, if successful, hard limit allocate will also be set
     * HINT: a not contained limit is no limit
     *
     * @param smf soft metred feature that should check for available allocation without warning
     * @param alloc some resource
     * @return if allocation was successful and metered counter updated without warning required
     */
    bool allocate(license_package::soft_metered_feature smf, std::size_t alloc);

    /**
     *
     * @param hmf hard metred feature to be deallocated
     * @param dealloc resources to be registered deallocated
     */
    void deallocate(soft_metered_feature hmf, std::size_t dealloc);

    /**
     *
     * @param hmf metred feature to test free count on
     * @return number of free resources of metred resource
     */
    std::size_t free_count(soft_metered_feature hmf);

    /**
     *
     * @param smf soft metered feature to be registered
     * @param smr soft metered resource class to be checked repeatedly
     */
    void add_metred_feature(license_package::soft_metered_feature smf, const std::shared_ptr<soft_metered_resource>& smr);

    /**
     *
     * @param smf metered feature to check
     * @return if soft metred feature is available
     */
    bool has_metred_feature(license_package::soft_metered_feature smf);

    /**
     *
     * @return if license checker returns valid
     */
    bool valid();

private:

    std::map<feature, bool> m_features;
    std::map<soft_metered_feature, std::shared_ptr<soft_metered_resource>> m_soft_metered_features;
    std::shared_ptr<check_airgap_license> m_check_license;

    /**
     * activates online defined features and limits
     */
    void feature_activation();
};

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
