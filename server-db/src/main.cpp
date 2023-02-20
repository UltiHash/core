#include <exception>

#include <config.hpp>
#include <storage/mod.h>
#include <storage/options.h>
#include <server/mod.h>
#include <metrics/mod.h>
#include <logging/logging_boost.h>

#include <options/app_config.h>
#include <options/metrics_options.h>
#include <options/logging_options.h>


APPLICATION_CONFIG(
    (server, uh::options::server_options),
    (logging, uh::options::logging_options),
    (metrics, uh::options::metrics_options),
    (storage, uh::dbn::storage::options));

using namespace uh::log;
using namespace uh::dbn;

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
        metrics::mod metrics_module(config.metrics()); //TODO add storage metrics

        storage::mod storage_module(config.storage(), metrics_module.storage());
        storage_module.start();

        server::mod server_module(config.server(), storage_module, metrics_module);
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
