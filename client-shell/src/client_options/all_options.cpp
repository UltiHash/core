#include "all_options.h"
#include "../project_config.h"

namespace po = boost::program_options;

namespace uh::client
{

// ---------------------------------------------------------------------

all_options::all_options()
{
    m_basic.apply(*this);
    m_client.apply(*this);
    m_agency.apply(*this);
}

// ---------------------------------------------------------------------

void all_options::parse(int argc, const char **argv)
{
    try
    {
        po::positional_options_description pos_opts_desc;
        pos_opts_desc.add("positional",-1);
        po::store(po::command_line_parser(argc, argv).options(m_desc).positional(pos_opts_desc).run(), m_vars);
        po::notify(m_vars);
        evaluate(m_vars);
    }
    catch(const po::too_many_positional_options_error &exc)
    {
        throw std::invalid_argument("Unknown arguments. Use --help to see the all the supported options.");
    }
}

// ---------------------------------------------------------------------

void all_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
    if (!this->basic().print_version() && !this->basic().print_help())
        m_client.evaluate(vars);
}

// ---------------------------------------------------------------------

bool all_options::handle()
{
    bool exit = false;

    if (this->basic().print_version())
    {
        std::cout << "version: " << PROJECT_NAME << " " << PROJECT_VERSION << "\n";
        exit = true;
    }

    if (this->basic().print_help())
    {
        std::cout << PROJECT_NAME << " is the UltiHash's command line interface. It can be used to integrate or read data from the UltiHash cluster.\n\n"
                                     << "General Usage:\n\n  " << PROJECT_NAME << " <--CLIENT_OPTION> <Path to UltiHash Volume> [Other Paths]... <--CONNECTION_OPTION arg>" << "\n\n  <> = required, [] = optional \n";
        boost::program_options::options_description visible;
        this->dump(std::cout);
        std::cout << '\n';
        exit = true;
    }
    else if (!exit)
    {
        m_agency.handle(m_vars, m_config);
        m_client.handle(m_vars, m_config);
    }

    return exit;
}

// ---------------------------------------------------------------------

const uh::options::basic_options& all_options::basic() const
{
    return m_basic;
}

// ---------------------------------------------------------------------

const client_options& all_options::client() const
{
    return m_client;
}

// ---------------------------------------------------------------------

const client_config& all_options::cl_config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::client
