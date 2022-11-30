#ifndef OPTIONS_SERVER_OPTIONS_H
#define OPTIONS_SERVER_OPTIONS_H

#include <net/server.h>
#include <options/basic_options.h>


namespace uh::options
{

// ---------------------------------------------------------------------

class server_options : public options
{
public:
    server_options();

    void apply(options& opts);
    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    const net::server_config& config() const;

private:
    net::server_config m_config;
    boost::program_options::options_description m_desc;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
