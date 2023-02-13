#ifndef OPTIONS_SERVER_OPTIONS_H
#define OPTIONS_SERVER_OPTIONS_H

#include <net/plain_server.h>
#include <options/basic_options.h>


namespace uh::options
{

// ---------------------------------------------------------------------

class server_options : public options
{
public:
    server_options();

    virtual action evaluate(const boost::program_options::variables_map& vars) override;

    const net::server_config& config() const;

private:
    net::server_config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
