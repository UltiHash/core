#ifndef OPTIONS_CONFIG_FILE_H
#define OPTIONS_CONFIG_FILE_H

#include "options.h"

#include <string>
#include <vector>


namespace uh::options
{

// ---------------------------------------------------------------------

class config_file : public options
{
public:
    config_file();

    const std::vector<std::string>& paths() const;
private:
    std::vector<std::string> m_config_paths;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
