#include "utils.h"

#include "common/telemetry/log.h"
#include <thread>

namespace uh::cluster {

std::unique_ptr<etcd::SyncClient> make_etcd_client(const etcd_config& cfg) {
    while (true) {
        try {
            if (cfg.username && cfg.password) {
                auto client = std::make_unique<etcd::SyncClient>(
                    cfg.url, *cfg.username, *cfg.password);
            }

            auto client = std::make_unique<etcd::SyncClient>(cfg.url);
            (void)(volatile etcd::Response)client->head();
            return client;
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to create etcd::SyncClient: " << e.what();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

} // namespace uh::cluster
