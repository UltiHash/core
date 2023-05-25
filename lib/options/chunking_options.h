#ifndef UH_OPTIONS_CHUNKING_OPTIONS_H
#define UH_OPTIONS_CHUNKING_OPTIONS_H

#include <options/options.h>
#include <chunking/mod.h>


namespace uh::options
{

// ---------------------------------------------------------------------

class chunking_options : public options
{
public:
    chunking_options();

    virtual action evaluate(const boost::program_options::variables_map& vars) override;

    const chunking::config& config() const;

private:
    chunking::config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
