#ifndef OPTIONS_LOADER_H
#define OPTIONS_LOADER_H

#include <options/options.h>

#include <boost/program_options/variables_map.hpp>

#include <filesystem>
#include <iosfwd>
#include <list>


namespace uh::options
{

// ---------------------------------------------------------------------

class loader
{
public:
    action parse(int argc, const char** argv);
    void parse(const std::filesystem::path& path);
    void parse(std::istream& in);

    loader& add(options& opt);

    const boost::program_options::options_description& visible() const;

private:
    std::list<options*> m_opts;
    action finalize(boost::program_options::variables_map& vars);

    boost::program_options::options_description m_hidden;
    boost::program_options::options_description m_visible;
    boost::program_options::options_description m_file;

    std::list<options::positional> m_positional_mappings;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
