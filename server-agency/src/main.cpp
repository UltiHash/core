#include <exception>
#include <iostream>

#include <options/basic_options.h>
#include <logging/logging_boost.h>
#include <config.hpp>
#include "protocol.h"
#include "server.h"


namespace uh::an
{

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    uh::options::basic_options& basic();
private:
    uh::options::basic_options m_basic;
};

// ---------------------------------------------------------------------

options::options()
{
    m_basic.apply(*this);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
}

// ---------------------------------------------------------------------

uh::options::basic_options& options::basic()
{
    return m_basic;
}

// ---------------------------------------------------------------------

class protocol_factory : public util::factory<protocol>
{
public:
    virtual std::unique_ptr<protocol> create() const override
    {
        return std::make_unique<uh_protocol>();
    }
};

// ---------------------------------------------------------------------

}

int main(int argc, const char** argv)
{
    try
    {
        uh::an::options options;
        options.parse(argc, argv);

        bool exit = false;

        if (options.basic().print_help())
        {
            options.dump(std::cout);
            std::cout << "\n";
            exit = true;
        }

        if (options.basic().print_version())
        {
            std::cout << "version: " << PROJECT_NAME << " " << PROJECT_VERSION << "\n";
            exit = true;
        }

        if (options.basic().print_vcsid())
        {
            std::cout << "vcsid: " << PROJECT_REPOSITORY << " - " << PROJECT_VCSID << "\n";
            exit = true;
        }

        if (exit)
        {
            return 0;
        }

        INFO << "starting server";
        uh::an::server_config config;
        uh::an::protocol_factory pf;
        uh::an::server srv(config, pf);

        srv.run();
    }
    catch (const std::exception& e)
    {
        FATAL << e.what() << "\n";
    }
    return 0;
}
