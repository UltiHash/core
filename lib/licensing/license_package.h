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
#include <vector>
#include <filesystem>
#include <algorithm>

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

            delete check_lic;
        }

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
        explicit license_package(const check_license::role license_role, const std::vector<feature> features_input = {},
                                 const std::filesystem::path& config = std::filesystem::current_path().parent_path())
        {
            //TODO: move license parser to check license and only make valid abstract

            for(uint8_t feature_iterate = 0; feature_iterate < feature_count_global; feature_iterate++){
                features.emplace((feature)feature_iterate,
                                 std::find(features_input.cbegin(),features_input.cend(),
                                           (feature)feature_iterate) != features_input.cend());
            }

            for(auto& file_object: std::filesystem::directory_iterator(config))
            {
                if(file_object.is_regular_file() && file_object.path().extension() == ".lic")
                {
                    //TODO: add licensing configuration how the license should be checked, license parser
                    auto* tmp_license_check = new check_online_license(file_object.path());

                    if(tmp_license_check->check_role() == license_role && tmp_license_check->valid()){
                        check_lic = tmp_license_check;
                        break;
                    }
                    else delete tmp_license_check;
                }
            }
        }

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
         * @throw if license role does not match requested role
         */
        void check_role_enabled(check_license::role r) const
        {
            if(check_lic->check_role() != r)
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
         * first try soft limit allocate, if successful, hard limit allocate will also be set
         * HINT: a not contained limit is no limit
         *
         * @param hmf hard metred feature that should check for available allocation
         * @param alloc some resource
         * @return if allocation was successful and metered counter updated --> valid operation
         */
        bool hard_limit_allocate(metered_feature hmf, std::size_t alloc)
        {
            if(!hard_metred_features.contains(hmf))
                return true;

            return hard_metred_features.at(hmf).hard_limit_allocate(alloc);
        }

        /**
         * first try soft limit allocate, if successful, hard limit allocate will also be set
         * HINT: a not contained limit is no limit
         *
         * @param smf soft metred feature that should check for available allocation without warning
         * @param alloc some resource
         * @return if allocation was successful and metered counter updated without warning required
         */
        bool soft_limit_allocate(soft_metered_feature smf, std::size_t alloc)
        {
            if(!hard_metred_features.contains(smf))
                return true;

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
                if(!hard_metred_features.contains(hmf))
                    return;

                hard_metred_features.at(hmf).deallocate(dealloc);
            }
        }

        /**
         *
         * @return if registered license type is valid
         */
        [[nodiscard]] bool valid()
        {
            return check_lic->valid();
        }

    protected:

        /**
         *
         * @param mf metered feature to be registered
         * @param mr metered resource class to be checked repeatedly
         */
        static void add_hard_metred_feature(metered_feature mf,metred_resource* mr)
        {
            hard_metred_features.emplace(mf,mr);
        }

        /**
         *
         * @param smf soft metered feature to be registered
         * @param smr soft metered resource class to be checked repeatedly
         */
        static void add_soft_metred_feature(soft_metered_feature smf,soft_metred_resource* smr)
        {
            soft_metred_features.emplace(smf,smr);
        }

    private:

        std::map<feature,bool> features;
        static std::map<metered_feature, metred_resource*> hard_metred_features;
        static std::map<soft_metered_feature, soft_metred_resource*> soft_metred_features;

        CHECK_METHOD* check_lic{};
        std::filesystem::path license_path;
        const uint8_t feature_count_global = 2;
    };

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
