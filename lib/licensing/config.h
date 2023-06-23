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

    backend_type type = license_spring;
    license_spring_config ls_config;

    std::string activation_key;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif
