#include "utils.h"

namespace uh::cluster {

etcd::SyncClient make_etcd_client(const etcd_config& cfg) {
    if (cfg.username && cfg.password) {
        return etcd::SyncClient(cfg.url, *cfg.username, *cfg.password);
    }

    return etcd::SyncClient(cfg.url);
}

} // namespace uh::cluster
