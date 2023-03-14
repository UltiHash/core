#ifndef OPTIONS_BASIC_HPP
#define OPTIONS_BASIC_HPP

#include "options.h"


namespace uh::options
{

// ---------------------------------------------------------------------

struct config
{
    bool help = false;
    bool version = false;
    bool vcsid = false;
};

// ---------------------------------------------------------------------

class basic_options : public options
{
public:
    basic_options();
    virtual action evaluate(const boost::program_options::variables_map&) override;

    [[nodiscard]] const uh::options::config& config() const;

private:
    uh::options::config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
