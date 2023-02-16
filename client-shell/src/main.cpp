#include "project_config.h"
#include "client_options/all_options.h"
#include "protocol/client_factory.h"
#include "protocol/client_pool.h"
#include <net/plain_socket.h>
#include <serialization/Recompilation.h>
#include <logging/logging_boost.h>

// ---------------------------------------------------------------------------------------------------------------------

int main(int argc, const char *argv[])
{
    try
    {
        // parse cli
        uh::client::option::all_options cli_options{};
        cli_options.parse(argc,argv);

        if (cli_options.handle()) {
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
        std::unique_ptr<uh::protocol::client_pool> client_pool =
            std::make_unique<uh::protocol::client_pool>(
                std::make_unique<uh::protocol::client_factory>(
                    std::make_unique<uh::net::plain_socket_factory>(
                                    io, cli_options.m_config.m_hostname, cli_options.m_config.m_port),
                                        cf_config), cli_options.m_config.m_pool_size);

        // recompilation
        uh::client::serialization::Recompilation(cli_options.m_config, std::move(client_pool));

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

// ---------------------------------------------------------------------------------------------------------------------
