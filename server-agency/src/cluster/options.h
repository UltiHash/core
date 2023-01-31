#ifndef SERVER_AGENCY_CLUSTER_OPTIONS_H
#define SERVER_AGENCY_CLUSTER_OPTIONS_H

#include <options/options.h>

#include "mod.h"


namespace uh::an::cluster
{

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    void apply(uh::options::options& opts);
    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    const cluster::config& config() const;

private:
    cluster::config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::an::cluster

#endif
