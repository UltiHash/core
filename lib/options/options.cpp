#include "options.h"

#include <boost/program_options/parsers.hpp>

#include <ostream>


namespace uh::options
{

// ---------------------------------------------------------------------

options::options(const std::string& caption)
    : m_visible(caption),
      m_hidden(caption)
{
}

// ---------------------------------------------------------------------

action options::evaluate(const boost::program_options::variables_map& vars)
{
    return action::proceed;
}

// ---------------------------------------------------------------------

const boost::program_options::options_description& options::hidden() const
{
    return m_hidden;
}

// ---------------------------------------------------------------------

const boost::program_options::options_description& options::visible() const
{
    return m_visible;
}

// ---------------------------------------------------------------------

boost::program_options::options_description& options::hidden()
{
    return m_hidden;
}

// ---------------------------------------------------------------------

boost::program_options::options_description& options::visible()
{
    return m_visible;
}

// ---------------------------------------------------------------------

void options::positional_mapping(const std::string& name, int max_count)
{
    m_positional_mappings.push_back({ name, max_count });
}

// ---------------------------------------------------------------------

const std::list<options::positional>& options::positional_mappings() const
{
    return m_positional_mappings;
}

// ---------------------------------------------------------------------

} // namespace uh::options
