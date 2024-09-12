#include "common/license/license.h"
#include "common/telemetry/log.h"
#include "common/telemetry/traces.h"
#include "common/utils/signal_handler.h"
#include "config/configuration.h"
#include "deduplicator/deduplicator.h"
#include "entrypoint/entrypoint.h"
#include "recovery/recovery.h"
#include "storage/storage.h"

using namespace uh;
using namespace uh::cluster;

void execute_role(const config& c) {

    signal_handler sh;

    auto start_service = [&sh](auto&& service) -> void {
        sh.add_callback([&service]() { service.stop(); });
        service.run();
    };

    try {
        switch (c.role) {
        case STORAGE_SERVICE:
            return start_service(storage(c.service, c.storage));
        case DEDUPLICATOR_SERVICE:
            return start_service(deduplicator(c.service, c.deduplicator));
        case ENTRYPOINT_SERVICE:
            return start_service(entrypoint(c.service, c.entrypoint));
        case RECOVERY_SERVICE:
            return start_service(recovery(c.service, c.recovery));
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in executing role: " << e.what();
        sh.stop();
    }
}

int main(int argc, char** argv) {

    try {
        auto config = read_config(argc, argv);
        if (!config) {
            return 0;
        }

        global_service_role = config->role;

        log::init(config->log);

        initialize_metrics_exporter(config->service.telemetry_url,
                                    config->service.telemetry_interval);
        initialize_traces_exporter(config->service.telemetry_url);

        LOG_INFO() << "license loaded for " << config->service.license.customer
                   << " -- storage size: "
                   << config->service.license.max_data_store_size << " bytes";

        execute_role(*config);
    } catch (const std::exception& e) {
        std::cerr << "Failure during startup: " << e.what() << "\n";
    }
}
