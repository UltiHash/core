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

    void apply(options& opts);
    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    const metrics::config& config() const;

private:
    metrics::config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
