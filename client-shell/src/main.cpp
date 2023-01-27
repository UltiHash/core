#include "project_config.h"
#include "client_options/all_options.h"
#include "protocol/client_factory.h"
#include <net/plain_socket.h>
#include <serialization/Recompilation.h>

int main(int argc, const char *argv[])
{
    try
    {
        // parse cli
        uh::client::all_options cli_options{};
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
        uh::protocol::client_factory client_factory(
                std::make_unique<uh::net::plain_socket_factory>(io, cli_options.m_config.m_hostname, cli_options.m_config.m_port),
                cf_config);

        // recompilation
//        uh::client::serialization::Recompilation(cli_options.m_config, client_factory.create());

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
