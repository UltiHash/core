#include "config.hpp"
#include "client_options/client_options.h"
#include "client_options/agency_connection.h"
#include "protocol/client_factory.h"
#include "protocol/client_pool.h"
#include <net/plain_socket.h>
#include <serialization/Recompilation.h>
#include <logging/logging_boost.h>
#include <options/app_config.h>

// ---------------------------------------------------------------------

APPLICATION_CONFIG(
    (client, uh::client::option::client_options),
    (agency, uh::client::option::agency_connection));

int main(int argc, const char *argv[])
{
    try
    {
        application_config config;
        if (config.evaluate(argc, argv) == uh::options::action::exit)
        {
            return 0;
        }

        // protocol
        boost::asio::io_context io;
        std::stringstream s;
        s << PROJECT_NAME << " " << PROJECT_VERSION;
        uh::protocol::client_factory_config cf_config
            {
                .client_version = s.str()
            };

        const auto& client_config = config.client();
        uh::protocol::client_factory client_factory(
                std::make_unique<uh::net::plain_socket_factory>(io, config.agency().hostname, config.agency().port),
                cf_config);
        std::unique_ptr<uh::protocol::client_pool> client_pool =
                std::make_unique<uh::protocol::client_pool>(
                        std::make_unique<uh::protocol::client_factory>(
                                std::make_unique<uh::net::plain_socket_factory>(
                                        io, config.agency().hostname, config.agency().port),
                                            cf_config), config.agency().pool_size);

        // recompilation
        uh::client::serialization::Recompilation(config.client(), std::move(client_pool));

    }
    catch (const std::exception &exc)
    {
        FATAL << exc.what() << '\n';

        return 1;
    }
    catch (...)
    {
        FATAL << "unknown exception occurred";
        return 1;
    }
    return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------
