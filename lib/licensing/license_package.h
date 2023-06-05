//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSE_PACKAGE_H
#define CORE_LICENSE_PACKAGE_H

#include "licensing/metred_resource.h"
#include "licensing/soft_metred_resource.h"
#include "licensing/check_online_license.h"
#include "util/exception.h"

#include <map>

namespace uh::licensing {

    template<class CHECK_METHOD = check_online_license>
    class license_package {

        ~license_package()
        {
            for(auto& item:hard_metred_features){
                delete item.second;
            }

            for(auto& item:soft_metred_features){
                delete item.second;
            }
        }

    public:

        /**
         *
         * @param config license file input
         * @throws if license is invalid or cannot be loaded
         */
        license_package(const license_package_config& config){

        }

        /**
         *
         * Check if `role` is enabled in the configured license.
         * @throw if `role`does not match given license
         */
        enum class role: unsigned char{
            AGENCY_NODE,
            DATA_NODE
        };

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
         * Check if `metered_feature is set in the configured license.
         * Limit the resource capacity to the value set by license.
         * Warn if resource boundary is close to be hit.
         * @throw if resource boundary will be exceeded by resource allocation
         */
        enum class metered_feature: unsigned char{
            LIMIT_CPU_COUNT = 1
        };

        /**
         *
         * Warn if resource boundary is close to be hit.
         * @throw if resource boundary will be exceeded by resource allocation
         */
        enum class soft_metered_feature: unsigned char{
            LIMIT_STORAGE_CAPACITY = 0,
            LIMIT_NETWORK_CONNECTIONS = 2
        };

        /**
         *
         * @param r role to be validated
         * @throw if license role does not match requested role
         */
        void check_role_enabled(role r) const
        {
            if(license_role != r)
                THROW(util::exception,"Requested role did not match license role!");
        }

        /**
         *
         * @param f check feature on or off/true or false
         * @throw feature state not specified by license and by that not listed
         * @return feature state
         */
        bool check_feature_enabled(feature f) const
        {
            return features.at(f);
        }

        /**
         *
         * @param hmf hard metred feature that should check for available allocation
         * @param alloc some resource
         * @return if allocation was successful and metered counter updated
         */
        bool hard_limit_allocate(metered_feature hmf, std::size_t alloc)
        {
            return hard_metred_features.at(hmf).hard_limit_allocate(alloc);
        }

        /**
         *
         * @param smf soft metred feature that should check for available allocation without warning
         * @param alloc some resource
         * @return if allocation was successful and metered counter updated without warning required
         */
        bool soft_limit_allocate(soft_metered_feature smf, std::size_t alloc)
        {
            return soft_metred_features.at(smf).soft_limit_allocate(alloc);
        }

        /**
         *
         * @param hmf hard metred feature to be deallocated
         * @param dealloc resources to be registered deallocated
         */
        void deallocate(metered_feature hmf, std::size_t dealloc)
        {
            if(soft_metred_features.contains(hmf)){
                soft_metred_features.at(hmf).deallocate(dealloc);
            }
            else{
                hard_metred_features.at(hmf).deallocate(dealloc);
            }
        }

    private:
        role license_role;
        std::map<feature,bool> features;
        std::map<metered_feature, metred_resource*> hard_metred_features;
        std::map<soft_metered_feature, soft_metred_resource*> soft_metred_features;
        CHECK_METHOD check_lic;
    };

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
