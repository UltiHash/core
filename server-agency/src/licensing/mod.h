//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_MOD_H
#define CORE_MOD_H

#include <licensing/license_package.h>
#include <options/licensing_options.h>

#include <memory>


namespace uh::an::licensing
{

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const uh::licensing::config& cfg);
    ~mod();

    static void start();

    uh::licensing::license_package& license_package();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::licensing

#endif //CORE_MOD_H
