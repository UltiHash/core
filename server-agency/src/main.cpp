#include <exception>

#include <config.hpp>
#include <cluster/mod.h>
#include <cluster/options.h>
#include <server/mod.h>
#include <metrics/mod.h>
#include <logging/logging_boost.h>

#include <options/app_config.h>
#include <options/metrics_options.h>
#include <options/logging_options.h>
#include <options/server_options.h>


APPLICATION_CONFIG(
    (server, uh::options::server_options),
    (logging, uh::options::logging_options),
    (metrics, uh::options::metrics_options),
    (cluster, uh::an::cluster::options));

using namespace uh::log;
using namespace uh::an;

int main(int argc, const char** argv)
{
    try
    {
        application_config config;
        if (config.evaluate(argc, argv) == uh::options::action::exit)
        {
            return 0;
        }

        init_logging(config.logging());

        INFO << "Setting up metrics";
        metrics::mod metrics_module(config.metrics());

        cluster::mod cluster_module(config.cluster());
        cluster_module.start();

        server::mod server_module(config.server(), cluster_module, metrics_module);
        server_module.start();
    }
    catch (const std::exception& e)
    {
        FATAL << e.what();
        std::cerr << "Error while starting service: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        FATAL << "unknown exception occurred";
        std::cerr << "Error while starting service: unknown error\n";
        return 1;
    }

    return 0;
}
