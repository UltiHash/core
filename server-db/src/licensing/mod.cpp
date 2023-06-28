//
// Created by benjamin-elias on 15.06.23.
//

#include "mod.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <util/exception.h>


namespace uh::dbn::licensing
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

std::unique_ptr<uh::licensing::license_package> make_licensing(uh::licensing::config cfg)
{
    cfg.config.productId = LICENSE_PRODUCT_ID;
    cfg.config.appName = PROJECT_NAME;
    cfg.config.appVersion = PROJECT_VERSION;

    return std::make_unique<uh::licensing::license_package>(cfg);
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

std::unique_ptr<uh::licensing::license_package> mod::g_license_package = {};

// ---------------------------------------------------------------------

mod::mod(const uh::licensing::config& cfg)
{
    g_license_package = make_licensing(cfg);
}

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting licensing module";
}

// ---------------------------------------------------------------------

uh::licensing::license_package &mod::license_package()
{
    return *g_license_package;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing
