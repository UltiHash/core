//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_LICENSING_OPTIONS_H
#define CORE_LICENSING_OPTIONS_H

#include <options/options.h>


namespace uh::options
{

// ---------------------------------------------------------------------

struct licensing_config
{
    std::string path;
    std::string key;
};

// ---------------------------------------------------------------------

class licensing_options : public uh::options::options
{
public:
    licensing_options();

    [[nodiscard]] const licensing_config& config() const;

protected:
    licensing_config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif //CORE_LICENSING_OPTIONS_H
