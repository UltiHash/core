//
// Created by benjamin-elias on 15.06.23.
//

#include "mod.h"

namespace uh::dbn::licensing
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

void maybe_create_license_root_directory(std::filesystem::path license_root)
{
    //if the path is a file path to license try to create parent
    if (license_root.extension() == ".lic")
        license_root = license_root.parent_path();

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
    auto it = std::find_if(uh::licensing::string2licensetype.begin(),
                           uh::licensing::string2licensetype.end(),
                           [&license_type](std::pair<std::string, uh::licensing::LicenseTypeEnum> &item)
                           {
                               return license_type == item.first;
                           });
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

    uh::licensing::LicenseTypeEnum license_type = define_licensing_type(cfg.license_type);

    auto lic_config = uh::licensing::license_config(license_type,
                                                    uh::licensing::NodeRole::DataNode,
                                                    cfg.licensing_path,
                                                    cfg.license_replace);

    auto api = uh::licensing::api_config(EncryptStr(LICENSE_API_KEY),
                                         EncryptStr(LICENSE_SHARED_KEY),
                                         EncryptStr(LICENSE_PRODUCT_ID));
    auto credential = uh::licensing::credential_config(PROJECT_NAME, PROJECT_VERSION);

    switch (license_type)
    {
        case uh::licensing::LicenseTypeEnum::AirgapKeyOnline:
        {
            auto activate = uh::licensing::license_activate_config(cfg.license_key);
            if (std::filesystem::is_empty(cfg.licensing_path))
            {
                INFO << "No licenses were found. Creating " + cfg.license_type + " license.";

                uh::licensing::check_airgap_license write_airgap(lic_config,
                                                                 api,
                                                                 credential,
                                                                 activate);

                INFO << "Initialized " + cfg.license_type;
                INFO << "Wrote new license key to " + lic_config.license_path.string();
            }

            return std::make_unique<uh::licensing::license_package>(
                std::make_shared<uh::licensing::check_airgap_license>(lic_config,
                                                                      api,
                                                                      credential,
                                                                      activate
                )
            );
        }
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
