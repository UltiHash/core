//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_LICENSING_OPTIONS_H
#define CORE_LICENSING_OPTIONS_H

#include <options/options.h>
#include <licensing/config.h>


namespace uh::options
{

// ---------------------------------------------------------------------

class licensing_options : public uh::options::options
{
public:
    licensing_options();

    [[nodiscard]] const licensing::config& config() const;

protected:
    licensing::config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif //CORE_LICENSING_OPTIONS_H
