//
// Created by benjamin-elias on 15.06.23.
//

#include "mod.h"
#include "licensing/license_package.h"

#include <config.hpp>

#include <unistd.h> //rmdir
#include <logging/logging_boost.h>
#include <util/exception.h>
#include <licensing/soft_metred_storage_resource.h>
#include <licensing/license_package.h>

#include <LicenseSpring/EncryptString.h>

namespace uh::dbn::licensing {

// ---------------------------------------------------------------------    

namespace
{

// ---------------------------------------------------------------------

void maybe_create_license_root_directory(std::filesystem::path license_root,
                                          bool ok_create_new_root){


    //Check whether the directory already exists:
    bool no_license_root = !std::filesystem::is_directory(license_root);

    //Check whether we want to create a new dir:
    if(no_license_root and not ok_create_new_root)
        THROW(util::exception, "Path does not exist: " + license_root.string());

    //We are OK creating a new root if needed, otherwise just inform about its existence
    if (no_license_root){
        if(!std::filesystem::create_directories(license_root)){
            std::string msg("Unable to create path for license root: " + license_root.string());
            THROW(util::exception, msg);
        }
        INFO << "Created new license root at " << license_root;
    }
    else{
        INFO << "Found existing license root at " << license_root;
    }
}

// ---------------------------------------------------------------------

LicenseTypeEnum define_licensing_type(const std::string& backend_type){
    auto it = string2backendtype.find(backend_type);
    if (it != string2backendtype.end()) {
        return it->second;
    } else {
        std::string msg("Not a licensing type: " + backend_type);
        THROW(util::exception, msg);
    }
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::licensing::license_package> make_licensing(const licensing_config& cfg)
{

    maybe_create_license_root_directory(cfg.license_root, cfg.create_new_license);

    auto license_type = define_licensing_type(cfg.license_type);

    switch (license_type)
    {
        case LicenseTypeEnum::AirgapOnlineActivationLicense:{
            uh::licensing::check_airgap_license write_airgap(cfg.license_root,EncryptStr(LICENSE_API_KEY),
                                                EncryptStr(LICENSE_SHARED_KEY),
                                                EncryptStr(LICENSE_PRODUCT_ID)
                                                );

            write_airgap.write_license(uh::licensing::check_license::role::DATA_NODE, PROJECT_NAME, PROJECT_VERSION,);

            return std::make_unique<uh::licensing::license_package>(uh::licensing::check_license::role::DATA_NODE,
                                                                    cfg.license_root,
                                                                    EncryptStr(LICENSE_API_KEY),
                                                                    EncryptStr(LICENSE_SHARED_KEY),
                                                                    EncryptStr(LICENSE_PRODUCT_ID));
        }
        case LicenseTypeEnum::OtherLicense:
            THROW(util::exception, "Not yet implemented licensing model");
    }

    std::string msg("Not a storage backend type: " + cfg.license_type);
    THROW(util::exception, msg);

}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
explicit impl(const licensing_config& cfg);
std::unique_ptr<uh::licensing::license_package> m_licensing;
};

// ---------------------------------------------------------------------

mod::impl::impl(const licensing_config& cfg)
    : m_licensing(make_licensing(cfg))
{
}

// ---------------------------------------------------------------------

mod::mod(const licensing_config& cfg)
    : m_impl(std::make_unique<impl>(cfg))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
INFO << "          starting licensing module";

if(!m_impl->m_licensing->valid())
    THROW(util::exception,"UltiHash license was not valid!");

}

// ---------------------------------------------------------------------

uh::licensing::license_package& mod::license_package()
{
return *m_impl->m_licensing;
}

// ---------------------------------------------------------------------
    
} // namespace uh::dbn::licensing