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
    explicit mod(const uh::options::licensing_config &cfg);

    ~mod();

    void start();

    uh::licensing::license_package &license_package();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing

#endif
