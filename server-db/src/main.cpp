#include <exception>

#include <config/mod.h>
#include <storage/mod.h>
#include <server/mod.h>
#include <metrics/mod.h>
#include <logging/logging_boost.h>


using namespace uh::log;
using namespace uh::dbn;

int main(int argc, const char** argv)
{
    try
    {
        uh::dbn::config::mod config_module(argc, argv); 

        if (config_module.handle())
        {
            return 0;
        }

        const auto& options = config_module.options();

        init_logging(options.logging().config());

        INFO << "Setting up metrics";
        metrics::mod metrics_module(options); //TODO add storage metrics

        storage::mod storage_module(options.storage().config(), metrics_module.storage());
        storage_module.start();

        server::mod server_module(options, storage_module, metrics_module);
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
