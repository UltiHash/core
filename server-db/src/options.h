#ifndef SERVER_DB_OPTIONS_H
#define SERVER_DB_OPTIONS_H

#include <options/basic_options.h>
#include <options/server_options.h>


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
private:
    uh::options::basic_options m_basic;
    uh::options::server_options m_server;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
