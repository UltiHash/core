//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_LICENSING_MOD_H
#define CORE_LICENSING_MOD_H

#include <licensing/license_package.h>
#include <options/licensing_options.h>

#include <memory>


namespace uh::dbn::licensing
{

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const uh::licensing::config& cfg);

    static void start();

    static uh::licensing::license_package& license_package();

private:
    static std::unique_ptr<uh::licensing::license_package> g_license_package;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing

#endif
