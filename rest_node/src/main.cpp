#include "server/server.h"

using namespace uh::rest;

int main(int argc, char* argv[])
{
    try
    {
        // command line parser
        if (argc != 4)
        {
            std::cerr <<
                      "Usage: http-server-awaitable <address> <port> <threads>\n" <<
                      "Example:\n" <<
                      "    http-server-awaitable 0.0.0.0 8080 1\n";
            return EXIT_FAILURE;
        }

        rest_server_config server_config;

        server_config.address = boost::asio::ip::make_address(argv[1]);
        server_config.port = static_cast<uint16_t>(std::atoi(argv[2]));
        server_config.threads = std::max<std::size_t>(1, std::atoi(argv[3]));

        // run the server; will block
        rest_server server(server_config);
        server.run();

    }
    catch (const std::exception &e)
    {
        FATAL << e.what();
        std::cerr << "Error while starting service: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...)
    {
        FATAL << "unknown exception occurred";
        std::cerr << "Error while starting service: unknown error\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
