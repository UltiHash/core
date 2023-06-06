//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSE_PACKAGE_H
#define CORE_LICENSE_PACKAGE_H

#include "licensing/metred_resource.h"
#include "licensing/soft_metred_resource.h"
#include "licensing/check_online_license.h"
#include "licensing/check_airgap_license.h"
#include "util/exception.h"

#include <map>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace uh::licensing {

    class license_package {

        ~license_package();

    public:

        /**
         *
         * Check if `feature`is enabled in the configured license.
         * Block feature if no license support is detected
         */

        enum class feature: unsigned char{
            METRICS,
            DEDUPLICATION
        };

        /**
         *
         * @param config license file input
         * @throws if license is invalid or cannot be loaded
         */
        explicit license_package(check_license::role license_role, const std::vector<feature>& features_input = {},
                                 const std::filesystem::path& config = std::filesystem::current_path().parent_path());

        /**
         *
         * Check if `metered_feature is set in the configured license.
         * Limit the resource capacity to the value set by license.
         * Warn if resource boundary is close to be hit.
         * @throw if resource boundary will be exceeded by resource allocation
         */
        enum class metered_feature: unsigned char{
            LIMIT_CPU_COUNT = 0
        };

        /**
         *
         * Warn if resource boundary is close to be hit.
         * @throw if resource boundary will be exceeded by resource allocation
         */
        enum class soft_metered_feature: unsigned char{
            LIMIT_STORAGE_CAPACITY = 1,
            LIMIT_NETWORK_CONNECTIONS = 2
        };

        /**
         *
         * @param r role to be validated
         * @throw if license role does not match requested role or license not loaded
         */
        void check_role_enabled(check_license::role r);

        /**
         *
         * @param l license to be validated
         * @throw if license type does not match requested license type or license not loaded
         */
        void check_license_enabled(check_license::license_type l);

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
        bool hard_limit_allocate(metered_feature hmf, std::size_t alloc);

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
        void deallocate(metered_feature hmf, std::size_t dealloc);

        /**
         *
         * @return if registered license type is valid
         */
        [[nodiscard]] bool valid();

    protected:

        /**
         *
         * @param mf metered feature to be registered
         * @param mr metered resource class to be checked repeatedly
         */
        void add_hard_metred_feature(metered_feature mf,metred_resource* mr);

        /**
         *
         * @param smf soft metered feature to be registered
         * @param smr soft metered resource class to be checked repeatedly
         */
        void add_soft_metred_feature(soft_metered_feature smf,soft_metred_resource* smr);

    private:

        std::map<feature,bool> features;
        std::map<metered_feature, metred_resource*> hard_metered_features;
        std::map<soft_metered_feature, soft_metred_resource*> soft_metered_features;

        check_license* check_lic{};
        std::filesystem::path license_path;
        const uint8_t feature_count_global = 2;
    };

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
