#ifndef OPTIONS_DATABASE_OPTIONS_H
#define OPTIONS_DATABASE_OPTIONS_H

#include <net/server.h>
#include <options/basic_options.h>
#include "db_config.h"


#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace uh::options
{

// ---------------------------------------------------------------------

class database_options : public options
{
public:
    database_options();

    void apply(options& opts);
    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    const uh::dbn::db_config& config() const;

private:
    uh::dbn::db_config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
