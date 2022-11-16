#ifndef OPTIONS_BASIC_HPP
#define OPTIONS_BASIC_HPP

#include "options.hpp"


namespace uh::options
{

// ---------------------------------------------------------------------

class basic_options
{
public:
    basic_options();
    void apply(options& opts);
    virtual void evaluate(const boost::program_options::variables_map& vars);

    bool print_help() const;
    bool print_version() const;
    bool print_vcsid() const;

private:
    boost::program_options::options_description m_desc;

    bool m_help = false;
    bool m_version = false;
    bool m_vcsid = false;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
