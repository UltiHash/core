#include <exception>

#include <config/mod.h>
#include <cluster/mod.h>
#include <server/mod.h>
#include <metrics/mod.h>
#include <logging/logging_boost.h>


using namespace uh::log;
using namespace uh::an;

int main(int argc, const char** argv)
{
    try
    {
        uh::an::config::mod config_module(argc, argv);

        if (config_module.handle())
        {
            return 0;
        }

        const auto& options = config_module.options();

        init_logging(options.logging().config());

        INFO << "Setting up metrics";
        metrics::mod metrics_module(options);

        cluster::config cluster_config {
            .nodes = {
                { .hostname = "localhost", .port = 12345 },
                { .hostname = "localhost", .port = 12346 },
                { .hostname = "localhost", .port = 12347 },
            }
        };

        cluster::mod cluster_module(cluster_config);
        cluster_module.start();

        server::mod server_module(options,
                                  cluster_module,
                                  metrics_module.metrics());
        server_module.start();
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
