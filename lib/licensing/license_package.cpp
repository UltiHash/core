//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/license_package.h"

namespace uh::licensing{

    // ---------------------------------------------------------------------

    license_package::~license_package() {
        delete check_lic;
    }

    // ---------------------------------------------------------------------

    license_package::license_package(check_license::role license_role,
                                     const std::set<feature>& features_input,
                                     const std::filesystem::path &config, const std::string& apiKey,
                                     const std::string& sharedKey, const std::string& productId)
        {
        for(uint8_t feature_iterate = 0; feature_iterate < feature_count_global; feature_iterate++){
            features.emplace((feature)feature_iterate,
                             std::find(features_input.cbegin(),features_input.cend(),
                                       (feature)feature_iterate) != features_input.cend());
        }

        auto init_lambda = [&apiKey,&sharedKey,&productId,this]
                (const std::filesystem::path& license_path_input)
                {
            auto tmp_license_type_read = check_license(license_path_input,
                                                       check_license::license_type::
                                                       AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION,
                                                       std::string(), std::string(),
                                                       std::string(),
                                                       "INIT_APP", "0.0.0");

            auto license_type_checked = tmp_license_type_read.check_license_type();

            switch (license_type_checked) {

                case check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION:
                {
                    auto* tmp_license_valid_airgap = new check_airgap_license(license_path_input,
                                                                              apiKey,
                                                                              sharedKey,
                                                                              productId);

                    if(tmp_license_valid_airgap->valid())check_lic = tmp_license_valid_airgap;
                    else delete tmp_license_valid_airgap;

                    break;
                }

                case check_license::license_type::FLOATING_ONLINE_USER_LICENSE:
                {
                    auto* tmp_license_valid_online = new check_airgap_license(license_path_input,
                                                                              apiKey,
                                                                              sharedKey,
                                                                              productId);

                    if(tmp_license_valid_online->valid())check_lic = tmp_license_valid_online;
                    else delete tmp_license_valid_online;

                    break;
                }

                default:
                    break;
            }

            return license_type_checked;
        };

        if(std::filesystem::exists(config) and std::filesystem::is_regular_file(config))
        {
            if(config.extension() != ".lic")
                THROW(util::exception,"Named license file did not carry extension \".lic\" ! Did you really chose a"
                                      "license file?");

            check_license::license_type licenseTypeToCheck = init_lambda(config);
            check_role_enabled(license_role);
            if(licenseTypeToCheck == check_license::license_type::THROW_LICENSE_TYPE)
                THROW(util::exception,"License type checker failed to wrong type of license check!");

            return;
        }

        for(auto& file_object: std::filesystem::directory_iterator(config))
        {
            if(file_object.is_regular_file() && file_object.path().extension() == ".lic")
            {
                check_license::license_type licenseTypeToCheck = init_lambda(file_object.path());

                try{
                    this->license_path = file_object.path();
                    check_role_enabled(license_role);

                    if(licenseTypeToCheck != check_license::license_type::THROW_LICENSE_TYPE)
                        return;
                }
                catch(std::exception& e){}

                delete check_lic;
            }
        }
    }

    // ---------------------------------------------------------------------

    void license_package::check_role_enabled(check_license::role r) {
        if(check_lic == nullptr)
            THROW(util::exception,"No license was loaded on check_role_enabled!");

        if(check_lic->check_role() != r)
            THROW(util::exception,"Requested role did not match license role!");
    }

    // ---------------------------------------------------------------------

    void license_package::check_license_enabled(check_license::license_type l) {
        if(check_lic == nullptr)
            THROW(util::exception,"No license was loaded on check_license_enabled!");

        if(check_lic->check_license_type() != l)
            THROW(util::exception,"Requested role did not match license role!");
    }

    // ---------------------------------------------------------------------

    bool license_package::check_feature_enabled(license_package::feature f) const {
        return features.at(f);
    }

    // ---------------------------------------------------------------------

    bool license_package::hard_limit_allocate(license_package::hard_metered_feature hmf, std::size_t alloc) {
        if(!hard_metered_features.contains(hmf))
            THROW(util::exception,"Hard metred feature was not avialable!");

        return hard_metered_features.at(hmf)->hard_limit_allocate(alloc);
    }

    // ---------------------------------------------------------------------

    bool license_package::soft_limit_allocate(license_package::soft_metered_feature smf, std::size_t alloc) {
        if(!soft_metered_features.contains(smf))
            THROW(util::exception,"Soft metred feature was not avialable!");

        return soft_metered_features.at(smf)->soft_limit_allocate(alloc);
    }

    // ---------------------------------------------------------------------

    void license_package::deallocate(license_package::hard_metered_feature hmf, std::size_t dealloc) {
        if(soft_metered_features.contains(static_cast<const soft_metered_feature>(hmf))){
            soft_metered_features.at(static_cast<const soft_metered_feature>(hmf))->deallocate(dealloc);
        }
        else{
            if(!hard_metered_features.contains(hmf))
                return;

            hard_metered_features.at(hmf)->deallocate(dealloc);
        }
    }

    // ---------------------------------------------------------------------

    bool license_package::valid() {
        if(!check_lic)
            THROW(util::exception,"No license was loaded on valid check!");

        return check_lic->valid();
    }

    // ---------------------------------------------------------------------

    void license_package::add_hard_metred_feature(license_package::hard_metered_feature mf, metred_resource *mr) {
        hard_metered_features.emplace(mf, mr);
    }

    // ---------------------------------------------------------------------

    void license_package::add_soft_metred_feature(license_package::soft_metered_feature smf, soft_metred_resource *smr) {
        hard_metered_features.emplace(static_cast<hard_metered_feature>(smf),smr);
        soft_metered_features.emplace(smf, smr);
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing