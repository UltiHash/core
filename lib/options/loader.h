#ifndef OPTIONS_LOADER_H
#define OPTIONS_LOADER_H

#include <options/options.h>
#include <list>
#include <options/config_file.h>
#include <filesystem>

namespace uh::options
{

// ---------------------------------------------------------------------

class loader
{
public:
    virtual ~loader() = default;

    loader& evaluate(int argc, const char** argv);
    action finalize();

    loader& add(options& opt);

    [[nodiscard]] const boost::program_options::options_description& visible() const;

protected:
    void parse(const std::filesystem::path &path);

private:
    std::list<options*> m_opts;
    boost::program_options::options_description m_hidden;
    boost::program_options::options_description m_visible;
    std::list<options::positional> m_positional_mappings;

    boost::program_options::options_description m_options;
    boost::program_options::variables_map m_vars;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
