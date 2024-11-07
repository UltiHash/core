#include "utils.h"

#include "common/telemetry/log.h"

namespace uh::cluster {

std::unique_ptr<etcd::SyncClient> make_etcd_client(const etcd_config& cfg) {
    while (true) {
        try {
            if (cfg.username && cfg.password) {
                auto client = std::make_unique<etcd::SyncClient>(
                    cfg.url, *cfg.username, *cfg.password);
            }

            auto client = std::make_unique<etcd::SyncClient>(cfg.url);

            if (!client->head().is_ok()) {
                LOG_ERROR() << "Wait until etcd::SyncClient.head().is_ok() "
                               "returns true";
                sleep(1);
                continue;
            }
            return client;
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to create etcd::SyncClient: " << e.what();
            sleep(1);
        }
    }
}

} // namespace uh::cluster
