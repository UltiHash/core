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

    virtual uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

    const cluster::config& config() const;

private:
    cluster::config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::an::cluster

#endif
