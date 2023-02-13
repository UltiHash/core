#ifndef UH_OPTIONS_LOGGING_OPTIONS_H
#define UH_OPTIONS_LOGGING_OPTIONS_H

#include <logging/logging_boost.h>
#include <options/options.h>


namespace uh::options
{

// ---------------------------------------------------------------------

class logging_options : public options
{
public:
    logging_options();

    virtual action evaluate(const boost::program_options::variables_map& vars) override;

    const log::config& config() const;

private:
    log::config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
