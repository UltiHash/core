#ifndef SERVER_DB_OPTIONS_H
#define SERVER_DB_OPTIONS_H

#include <options/basic_options.h>
#include <options/server_options.h>
#include "database_options.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    uh::options::basic_options& basic();
    uh::options::server_options& server();
    uh::options::database_options& database();
private:
    uh::options::basic_options m_basic;
    uh::options::server_options m_server;
    uh::options::database_options m_database;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
