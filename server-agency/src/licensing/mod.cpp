//
// Created by benjamin-elias on 15.06.23.
//

#include "mod.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <util/exception.h>


namespace uh::an::licensing
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

std::unique_ptr<uh::licensing::license_package> make_licensing(uh::licensing::config cfg)
{
    cfg.ls_config.productId = LICENSE_PRODUCT_ID;
    cfg.ls_config.appName = PROJECT_NAME;
    cfg.ls_config.appVersion = PROJECT_VERSION;

    return std::make_unique<uh::licensing::license_package>(cfg);
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    explicit impl(const uh::licensing::config& cfg);
    std::unique_ptr<uh::licensing::license_package> m_licensing;
};

// ---------------------------------------------------------------------

mod::impl::impl(const uh::licensing::config& cfg)
    : m_licensing(make_licensing(cfg))
{
}

// ---------------------------------------------------------------------

mod::mod(const uh::licensing::config& cfg)
    : m_impl(std::make_unique<impl>(cfg))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "          starting licensing module";
}

// ---------------------------------------------------------------------

uh::licensing::license_package& mod::license_package()
{
    return *m_impl->m_licensing;
}

// ---------------------------------------------------------------------

} // namespace uh::an::licensing
