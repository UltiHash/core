#ifndef CLIENT_CHUNKING_OPTIONS_H
#define CLIENT_CHUNKING_OPTIONS_H

#include <options/options.h>

#include "mod.h"


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    virtual uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

    const chunking_config& config() const;

private:
    chunking_config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
