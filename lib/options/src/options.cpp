#include "../include/options.h"

#include <boost/program_options/parsers.hpp>

#include <ostream>


namespace po = boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

void options::parse(int argc, const char** argv)
{
    po::store(po::parse_command_line(argc, argv, m_desc), m_vars);
    po::notify(m_vars);
    evaluate(m_vars);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map&)
{
}

// ---------------------------------------------------------------------

void options::add(boost::program_options::options_description& desc)
{
    m_desc.add(desc);
}

// ---------------------------------------------------------------------

void options::dump(std::ostream& out) const
{
    out << m_desc;
}

// ---------------------------------------------------------------------

} // namespace uh::options
