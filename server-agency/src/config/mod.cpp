#include "mod.h"

#include <config.hpp>

#include <iostream>


namespace uh::an::config
{

// ---------------------------------------------------------------------

struct mod::impl
{
    config::options options;
};

// ---------------------------------------------------------------------

mod::mod(int argc, const char** argv)
    : m_impl(std::make_unique<impl>())
{
    m_impl->options.parse(argc, argv);
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

bool mod::handle() const
{
    bool exit = false;

    if (m_impl->options.basic().print_help())
    {
        m_impl->options.dump(std::cout);
        std::cout << "\n";
        exit = true;
    }

    if (m_impl->options.basic().print_version())
    {
        std::cout << "version: " << PROJECT_NAME << " " << PROJECT_VERSION << "\n";
        exit = true;
    }

    if (m_impl->options.basic().print_vcsid())
    {
        std::cout << "vcsid: " << PROJECT_REPOSITORY << " - " << PROJECT_VCSID << "\n";
        exit = true;
    }

    return exit;
}

// ---------------------------------------------------------------------

const an::config::options& mod::options() const
{
    return m_impl->options;
}

// ---------------------------------------------------------------------

} // namespace uh::an::config
