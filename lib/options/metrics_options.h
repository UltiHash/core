#ifndef OPTIONS_METRICS_OPTIONS_H
#define OPTIONS_METRICS_OPTIONS_H

#include <metrics/service.h>
#include <options/basic_options.h>


namespace uh::options
{

// ---------------------------------------------------------------------

class metrics_options : public options
{
public:
    metrics_options();

    const metrics::config& config() const;

private:
    metrics::config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
