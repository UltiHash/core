#include <exception>
#include <iostream>

#include <net/server.h>
#include <logging/logging_boost.h>
#include <config.hpp>
#include "options.h"
#include "protocol_factory.h"


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
        uh::an::protocol_factory pf;
        uh::net::server srv(options.server().config(), pf);

        srv.run();
    }
    catch (const std::exception& e)
    {
        FATAL << e.what();
        return 1;
    }
    catch (...)
    {
        FATAL << "unknown exception occured";
        return 1;
    }

    return 0;
}
