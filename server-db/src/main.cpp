#include <logging/logging_boost.h>

#include <config.hpp>
#include "options.h"
#include "protocol_factory.h"
#include "storage_backend.h"

#include <exception>
#include <iostream>
#include <string>


using namespace uh::dbn;
namespace fs = boost::filesystem;

dump_storage create_ultihash_storage_backend(options &options){
    uh::dbn::db_config config = options.database().config();
    if(!(fs::exists(config.db_root))) {
        // The root path does not exist. Should we create a new one?
        if(!config.create_new_root) {
            // We don't want to create a new root for the db;
            // throw an error if the root requested does not exist
            std::string msg("Path does not exist: " + config.db_root.string());
            throw std::runtime_error(msg);
        }else{
            // We want to create a new root.
            // If the root fails to be created, throw an error.
            if(!boost::filesystem::create_directories(config.db_root)){
                std::string msg("Unable to create path for database root: " + config.db_root.string());
                throw std::runtime_error(msg);
            }
            else{
                INFO << "Created new database root at " << config.db_root;
            }
        }
    }
    else{
        //The root path exists, inform about it:
        INFO << "Found existing database root at " << config.db_root;
    }
    dump_storage uhsb(config);
    return uhsb;
}

//TODO - Consider using mmap
int main(int argc, const char** argv)
{
    uh::dbn::options options;

    try
    {
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
    }
    catch (const std::exception& e)
    {
        FATAL << e.what() << "\n";
    }

    try {
        auto uhsb = create_ultihash_storage_backend(options);
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
