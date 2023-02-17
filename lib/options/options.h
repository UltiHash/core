#ifndef UH_OPTIONS_OPTIONS_HPP
#define UH_OPTIONS_OPTIONS_HPP

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <list>


namespace uh::options
{

// ---------------------------------------------------------------------

enum class action
{
    exit,
    proceed
};

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, const action& a);

// ---------------------------------------------------------------------

class options
{
public:
    struct positional
    {
        std::string map_to;
        int max_count;
    };

    options(const std::string& caption);

    virtual action evaluate(const boost::program_options::variables_map& vars);

    const boost::program_options::options_description& hidden() const;
    const boost::program_options::options_description& visible() const;
    const boost::program_options::options_description& file() const;

    boost::program_options::options_description& hidden();
    boost::program_options::options_description& visible();
    boost::program_options::options_description& file();

    void positional_mapping(const std::string& name, int max_count);
    const std::list<positional>& positional_mappings() const;
private:
    boost::program_options::options_description m_hidden;
    boost::program_options::options_description m_visible;
    boost::program_options::options_description m_file;

    std::list<positional> m_positional_mappings;
};

// ---------------------------------------------------------------------

} // namespace uh::options

// ---------------------------------------------------------------------

namespace std
{

// ---------------------------------------------------------------------

std::string to_string(uh::options::action action);

// ---------------------------------------------------------------------

} // namespace std

#endif
