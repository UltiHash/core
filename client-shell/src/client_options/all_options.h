#ifndef CLIENT_OPTIONS_ALL_OPTIONS_H
#define CLIENT_OPTIONS_ALL_OPTIONS_H

#include <options/basic_options.h>
#include <boost/program_options/parsers.hpp>
#include "client_options.h"
#include "client_config.h"


namespace uh::client
{

// ---------------------------------------------------------------------

class all_options : public uh::options::options
{

public:

    // CLASS FUNCTIONS
    all_options();

    // GETTERS
    [[nodiscard]] const uh::options::basic_options& basic() const;
    [[nodiscard]] const client_options& client() const;
    [[nodiscard]] const client_config& cl_config() const;

    // LOGIC FUNCTIONS
    void evaluate(const boost::program_options::variables_map& vars) override;
    void parse(int argc, const char** argv) override;
    [[nodiscard]] bool handle();

public:
    client_config m_config{0x5548};

private:
    uh::options::basic_options m_basic;
    client_options m_client;
    agency_connection m_agency;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
