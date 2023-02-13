#ifndef UH_OPTIONS_OPTIONS_HPP
#define UH_OPTIONS_OPTIONS_HPP

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>


namespace uh::options
{

// ---------------------------------------------------------------------

enum class action
{
    exit,
    proceed
};

// ---------------------------------------------------------------------

class options
{
public:
    options(const std::string& caption);

    virtual action evaluate(const boost::program_options::variables_map& vars);

    const boost::program_options::options_description& hidden() const;
    const boost::program_options::options_description& visible() const;

    boost::program_options::options_description& hidden();
    boost::program_options::options_description& visible();

private:
    boost::program_options::options_description m_hidden;
    boost::program_options::options_description m_visible;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
