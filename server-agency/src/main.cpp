#include <exception>

#include <config.hpp>
#include <cluster/mod.h>
#include <cluster/options.h>
#include <server/mod.h>
#include <metrics/mod.h>
#include <persistence/mod.h>
#include <persistence/options.h>
#include <logging/logging_boost.h>

#include <options/app_config.h>
#include <options/metrics_options.h>
#include <options/logging_options.h>
#include <options/server_options.h>

#include "signals/signal.h"

APPLICATION_CONFIG(
    (server, uh::options::server_options),
    (logging, uh::options::logging_options),
    (metrics, uh::options::metrics_options),
    (cluster, uh::an::cluster::options),
    (persistence, uh::an::persistence::options));

using namespace uh::log;
using namespace uh::an;

int main(int argc, const char** argv)
{
    try
    {
        uh::signal::signal signal_handler;

        application_config config;
        if (config.evaluate(argc, argv) == uh::options::action::exit)
        {
            return 0;
        }

        init_logging(config.logging());


        INFO << "               --- Agency Node Modules ---";
        cluster::mod cluster_module(config.cluster());
        cluster_module.start();

        uh::an::persistence::mod persistence_module(config.persistence());
        persistence_module.start();

        metrics::mod metrics_module(config.metrics(), persistence_module);

        server::mod server_module(config.server(), cluster_module, metrics_module);
        server_module.start();


        signal_handler.register_func([&](){ server_module.stop();
                                                        cluster_module.stop(); });

        auto signal_received = signal_handler.run();
        INFO << " agency node clean shutdown: signal " << strsignal(signal_received) << "(" << signal_received << ") ...";

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
