#include "options.h"

#include <util/exception.h>
#include <boost/program_options/parsers.hpp>

#include <ostream>


namespace uh::options
{

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, const action& a)
{
    out << std::to_string(a);
    return out;
}

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

const boost::program_options::options_description& options::file() const
{
    return m_file;
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

boost::program_options::options_description& options::file()
{
    return m_file;
}

// ---------------------------------------------------------------------

} // namespace uh::options

// ---------------------------------------------------------------------

namespace std
{

// ---------------------------------------------------------------------

std::string to_string(uh::options::action action)
{
    switch (action)
    {
        case uh::options::action::proceed: return "proceed";
        case uh::options::action::exit: return "exit";
    }

    THROW(uh::util::exception, "undefined action value");
}

// ---------------------------------------------------------------------

} // namespace std
