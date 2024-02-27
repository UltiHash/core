#include "common/license/license.h"
#include "common/telemetry/log.h"
#include "common/utils/common.h"
#include "common/utils/signal_handler.h"
#include "config/config.h"
#include "deduplicator/deduplicator.h"
#include "directory/directory.h"
#include "entrypoint/entrypoint.h"
#include "storage/storage.h"

using namespace uh;
using namespace uh::cluster;

void execute_role(cluster::role role, const service_config& cfg) {

    signal_handler sh;

    auto start_service = [&sh](auto&& service) -> void {
        sh.add_callback([&service]() { service.stop(); });
        service.run();
    };

    try {
        switch (role) {
        case STORAGE_SERVICE:
            return start_service(storage(cfg, {}));
        case DEDUPLICATOR_SERVICE:
            return start_service(deduplicator(cfg, {}));
        case DIRECTORY_SERVICE:
            return start_service(directory(cfg, update_config(directory_config{}, cfg.license)));
        case ENTRYPOINT_SERVICE:
            return start_service(entrypoint(cfg, {}));
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in executing role: " << e.what();
        sh.stop();
    }
}

int main(int argc, char** argv) {
    try {
        auto config = read_config(argc, argv);
        log::init(config.log);

        LOG_INFO() << "license loaded for " << config.service.license.customer
                   << " -- storage size: "
                   << config.service.license.max_data_store_size << " bytes";

        execute_role(config.role, config.service);
    } catch (const std::exception& e) {
        std::cerr << "Failure during startup: " << e.what() << "\n";
    }
}
