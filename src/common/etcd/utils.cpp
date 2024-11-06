#include "utils.h"

#include "common/telemetry/log.h"

namespace uh::cluster {

etcd::SyncClient make_etcd_client(const etcd_config& cfg) {
    while (true) {
        try {
            if (cfg.username && cfg.password) {
                return etcd::SyncClient(cfg.url, *cfg.username, *cfg.password);
            }

            return etcd::SyncClient(cfg.url);
        } catch (const std::exception& e) {
            LOG_INFO() << "Exception from etcd::SyncClient, " << e.what();
        }
    }
}

} // namespace uh::cluster
