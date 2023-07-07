#ifndef LICENSING_CONFIG_H
#define LICENSING_CONFIG_H

#include <licensing/common.h>


#ifdef USE_LICENSE_SPRING
#include <licensing/license_spring.h>
#endif


namespace uh::licensing
{

// ---------------------------------------------------------------------

struct config
{
    enum backend_type
    {
        license_spring,
        license_spring_demo
    };

    backend_type type = license_spring;
    license_config config;

    std::string activation_key;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif
