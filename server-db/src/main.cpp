#include <exception>

#include <config.hpp>
#include <storage/mod.h>
#include <storage/options.h>
#include <server/mod.h>
#include <state/mod.h>
#include <metrics/mod.h>
#include <licensing/mod.h>
#include <logging/logging_boost.h>

#include <storage/storage_config.h>
#include <options/app_config.h>
#include <options/metrics_options.h>
#include <options/logging_options.h>
#include <options/licensing_options.h>
#include <signals/signal.h>

#include <licensing/global_licensing.h>

APPLICATION_CONFIG(
    (server, uh::options::server_options),
    (logging, uh::options::logging_options),
    (metrics, uh::options::metrics_options),
    (storage, uh::dbn::storage::options),
    (comp, uh::dbn::storage::compression_options),
    (licensing, uh::options::licensing_options));

using namespace uh::log;
using namespace uh::dbn;

int main(int argc, const char **argv)
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

        INFO << "--- Database Node Modules ---";
        metrics::mod metrics_module(config.metrics()); //TODO add storage metrics

        uh::dbn::licensing::global_license_pointer_dbn = std::make_unique<uh::dbn::licensing::mod>(config.licensing());
        uh::dbn::licensing::global_license_pointer_dbn->start();

        auto storage_config = config.storage();
        state::mod state_module(storage_config);
        state_module.start();

        storage_config.comp = config.comp();
        storage::mod storage_module(storage_config, metrics_module.storage(),
                                    state_module.scheduled_compressions());
        storage_module.start();

        server::mod server_module(config.server(), storage_module, metrics_module);
        server_module.start();

        signal_handler.register_func([&]()
                                     {
                                         server_module.stop();
                                         storage_module.stop();
                                     });

        auto signal_received = signal_handler.run();
        INFO << "data node clean shutdown: signal " << strsignal(signal_received) << "(" << signal_received << ")";
    }
    catch (const std::exception &e)
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
