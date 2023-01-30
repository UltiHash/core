#ifndef UH_OPTIONS_OPTIONS_HPP
#define UH_OPTIONS_OPTIONS_HPP

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iosfwd>


namespace uh::options
{

// ---------------------------------------------------------------------

// tagging option descriptions for visibility
enum class visibility { hidden, visible };

// ---------------------------------------------------------------------

class options
{
public:
    virtual void parse(int argc, const char** argv);
    virtual void evaluate(const boost::program_options::variables_map& vars);
    void add(const boost::program_options::options_description& desc, const visibility& vis = visibility::visible);
    void dump(std::ostream& out) const;

protected:
    boost::program_options::options_description m_desc;
    boost::program_options::options_description m_visible;
    boost::program_options::variables_map m_vars;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
