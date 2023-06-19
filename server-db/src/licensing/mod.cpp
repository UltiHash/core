//
// Created by benjamin-elias on 15.06.23.
//

#include "mod.h"
#include "licensing/license_package.h"
#include "options/licensing_options.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <util/exception.h>

#include <LicenseSpring/EncryptString.h>

namespace uh::dbn::licensing
{

// ---------------------------------------------------------------------    

namespace
{

// ---------------------------------------------------------------------

void maybe_create_license_root_directory(std::filesystem::path license_root)
{
    //Check whether the directory already exists:

    //We are OK creating a new root if needed, otherwise just inform about its existence
    if (!std::filesystem::is_directory(license_root))
    {
        if (!std::filesystem::create_directories(license_root))
        {
            std::string msg("Unable to create path for license root: " + license_root.string());
            THROW(util::exception, msg);
        }
        INFO << "Created new license root at " << license_root;
    }
    else
    {
        INFO << "Found existing license root at " << license_root;
    }
}

// ---------------------------------------------------------------------

uh::licensing::LicenseTypeEnum define_licensing_type(const std::string &license_type)
{
    auto it = uh::licensing::string2licensetype.find(license_type);
    if (it != uh::licensing::string2licensetype.end())
    {
        return it->second;
    }
    else
    {
        std::string msg("Not a licensing type: " + license_type);
        INFO << msg;
        THROW(util::exception, msg);
    }
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::licensing::license_package> make_licensing(const uh::options::licensing_config &cfg)
{
    maybe_create_license_root_directory(cfg.licensing_path);

    auto license_type = define_licensing_type(cfg.license_type);

    switch (license_type)
    {
        case uh::licensing::LicenseTypeEnum::AirgapOnline:
        {
            if (std::filesystem::is_empty(cfg.licensing_path))
            {
                INFO << "No licenses were found. Creating " + cfg.license_type + " license.";

                uh::licensing::check_key_license write_airgap(cfg.licensing_path,
                                                              EncryptStr(LICENSE_API_KEY),
                                                              EncryptStr(LICENSE_SHARED_KEY),
                                                              EncryptStr(LICENSE_PRODUCT_ID),
                                                              PROJECT_NAME,
                                                              PROJECT_VERSION,
                                                              cfg.license_replace
                );

                INFO << "Initialized " + cfg.license_type;

                write_airgap.write_license(uh::licensing::check_license::role::DataNode,
                                           PROJECT_NAME,
                                           PROJECT_VERSION,
                                           cfg.license_key);

                INFO << "Wrote new license key to " + write_airgap.getLicensePath().string();
            }

            auto read_license =
                std::make_unique<uh::licensing::license_package>(uh::licensing::check_license::role::DataNode,
                                                                 cfg.licensing_path,
                                                                 EncryptStr(LICENSE_API_KEY),
                                                                 EncryptStr(LICENSE_SHARED_KEY),
                                                                 EncryptStr(LICENSE_PRODUCT_ID));

            return read_license;
        }
        case uh::licensing::LicenseTypeEnum::FloatingOnline:THROW(util::exception,
                                                                  "Not yet implemented licensing model");
        case uh::licensing::LicenseTypeEnum::OtherLicense:THROW(util::exception, "Not yet implemented licensing model");
    }

    std::string msg("Not a storage backend type: " + cfg.license_type);
    THROW(util::exception, msg);
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    explicit impl(const uh::options::licensing_config &cfg);
    std::unique_ptr<uh::licensing::license_package> m_licensing;
};

// ---------------------------------------------------------------------

mod::impl::impl(const uh::options::licensing_config &cfg)
    : m_licensing(make_licensing(cfg))
{
}

// ---------------------------------------------------------------------

mod::mod(const uh::options::licensing_config &cfg)
    : m_impl(std::make_unique<impl>(cfg))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "          starting licensing module";

    if (!m_impl->m_licensing->valid())
    THROW(util::exception, "UltiHash " + std::string(PROJECT_NAME) + " license was not valid!");

}

// ---------------------------------------------------------------------

uh::licensing::license_package &mod::license_package()
{
    return *m_impl->m_licensing;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing
