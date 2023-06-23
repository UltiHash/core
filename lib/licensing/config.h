#ifndef LICENSING_CONFIG_H
#define LICENSING_CONFIG_H

#include <licensing/license_spring.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

struct config
{
    enum backend_type
    {
        license_spring,
    };

    license_spring_config ls_airgap;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif
