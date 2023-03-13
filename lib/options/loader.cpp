#include "loader.h"

#include <boost/program_options/parsers.hpp>
#include <iostream>

namespace po = boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

action loader::evaluate(int argc, const char** argv)
{
    m_options.add(m_visible);
    m_options.add(m_hidden);

    po::positional_options_description pos;
    for (const auto pm : m_positional_mappings)
    {
        pos.add(pm.map_to.c_str(), pm.max_count);
    }

    po::command_line_parser parser(argc, argv);
    parser.options(m_options);
    parser.positional(pos);
    auto parsed = parser.run();
    po::store(parsed, m_vars);

}

// ---------------------------------------------------------------------

void loader::parse_config(const std::filesystem::path& path)
{
    po::store(po::parse_config_file(path.c_str(), m_options), m_vars);
}

// ---------------------------------------------------------------------

loader& loader::add(options& opt)
{
    m_opts.push_back(&opt);
    m_visible.add(opt.visible());
    m_hidden.add(opt.hidden());

    const auto& ps = opt.positional_mappings();
    m_positional_mappings.insert(m_positional_mappings.end(), ps.begin(), ps.end());
    return *this;
}

// ---------------------------------------------------------------------

const boost::program_options::options_description& loader::visible() const
{
    return m_visible;
}

// ---------------------------------------------------------------------

action loader::finalize()
{
    po::notify(m_vars);

    bool errors = false;
    action rv = action::proceed;
    for (const auto& opt : m_opts)
    {
        try
        {
            if (opt->evaluate(m_vars) == action::exit)
            {
                rv = action::exit;
            }
        }
        catch (const std::exception& e)
        {
            errors = true;
            std::cout << "error: " << e.what() << "\n";
        }
    }

    if (errors)
    {
        return action::exit;
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::options
