#ifndef OPTIONS_LOADER_H
#define OPTIONS_LOADER_H

#include <options/options.h>
#include <list>


namespace uh::options
{

// ---------------------------------------------------------------------

class loader
{
public:
    virtual ~loader() = default;

    virtual action evaluate(int argc, const char** argv);

    loader& add(options& opt);

    const boost::program_options::options_description& visible() const;

private:
    std::list<options*> m_opts;
    boost::program_options::options_description m_hidden;
    boost::program_options::options_description m_visible;

    std::list<options::positional> m_positional_mappings;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
