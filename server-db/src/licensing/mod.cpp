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

std::unique_ptr<uh::licensing::license_package> make_licensing(const uh::options::licensing_config& cfg)
{
    maybe_create_license_root_directory(cfg.path);

    auto config = uh::licensing::ls_airgap_config{
        .productId = LICENSE_PRODUCT_ID,
        .appName = PROJECT_NAME,
        .appVersion = PROJECT_VERSION,
        .path = cfg.path };

    return std::make_unique<uh::licensing::license_package>(
        std::make_shared<uh::licensing::check_airgap_license>(config, cfg.key)
    );
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
