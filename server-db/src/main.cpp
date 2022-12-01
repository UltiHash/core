#include <logging/logging_boost.h>

#include <config.hpp>
#include "options.h"
#include "protocol_factory.h"
#include "storage_backend.h"

#include <exception>
#include <iostream>
#include <string>


using namespace uh::dbn;

//TODO - Consider using mmap
int main(int argc, const char** argv)
{
    try
    {
        uh::dbn::options options;
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

        uh::dbn::db_config config;

        if(!(boost::filesystem::exists(config.db_dir))) {
            std::string msg("Path does not exist: " + config.db_dir.string());
            throw std::runtime_error(msg);
        }
        storage_backend uhsb(config);

        INFO << "starting server";
        uh::dbn::protocol_factory pf(uhsb);
        uh::net::server srv(options.server().config(), pf);

        srv.run();
    }
    catch (const std::exception& e)
    {
        FATAL << e.what() << "\n";
    }

    return 0;
}
