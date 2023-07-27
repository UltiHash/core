#include "server/server.h"

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
                  "Usage: http-server-awaitable <address> <port> <threads>\n" <<
                  "Example:\n" <<
                  "    http-server-awaitable 0.0.0.0 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Spawn a listening port
    boost::asio::co_spawn(ioc,
                          do_listen(tcp::endpoint{address, port}),
                          [](const std::exception_ptr& e)
                          {
                              if (e)
                                  try
                                  {
                                      std::rethrow_exception(e);
                                  }
                                  catch(std::exception & e)
                                  {
                                      std::cerr << "Error in acceptor: " << e.what() << "\n";
                                  }
                          });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
                [&ioc]
                {
                    ioc.run();
                });
    ioc.run();

    return EXIT_SUCCESS;
}
