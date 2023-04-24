#ifndef SERVER_DATABASE_STORAGE_OPTIONS_H
#define SERVER_DATABASE_STORAGE_OPTIONS_H

#include <options/options.h>
#include <util/exception.h>

#include "mod.h"


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    virtual uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

    const storage_config& config() const;

private:
    storage_config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
