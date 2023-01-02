#include <exception>
#include <iostream>

#include <metrics/service.h>
#include <net/server.h>
#include <net/tls_server.h>
#include <net/plain_socket.h>
#include <protocol/client_factory.h>
#include <logging/logging_boost.h>
#include <config.hpp>
#include "options.h"
#include "protocol_factory.h"


using namespace uh::log;

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

        init_logging(options.logging().config());

        INFO << "Setting up metrics";
        uh::metrics::service metrics(options.metrics().config());

        INFO << "starting server";
        boost::asio::io_context io;

        std::stringstream s;
        s << PROJECT_NAME << " " << PROJECT_VERSION;
        uh::protocol::client_factory_config cf_config
        {
            .client_version = s.str()
        };

        uh::net::plain_socket_factory socket_factory(io, "localhost", 12345);
        uh::protocol::client_factory client_factory(socket_factory, cf_config);

        uh::protocol::client_pool clients(client_factory, 10);
        uh::an::protocol_factory pf(clients);

        if (!options.server().config().tls_chain.empty())
        {
            uh::net::tls_server srv(options.server().config(), pf);
            srv.run();
        }
        else
        {
            uh::net::server srv(options.server().config(), pf);
            srv.run();
        }
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
