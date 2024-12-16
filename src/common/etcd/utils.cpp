#include "utils.h"

#include "common/telemetry/log.h"

namespace uh::cluster {

std::unique_ptr<etcd::SyncClient> make_etcd_client(const etcd_config& cfg) {
    while (true) {
        try {
            std::unique_ptr<etcd::SyncClient> client;
            if (cfg.username && cfg.password) {
                client = std::make_unique<etcd::SyncClient>(
                    cfg.url, *cfg.username, *cfg.password);
            } else {
                client = std::make_unique<etcd::SyncClient>(cfg.url);
            }

            if (!client->head().is_ok()) {
                LOG_INFO() << "cannot connect to etcd. trying to reconnect";
                sleep(1);
                continue;
            }
            return client;
        } catch (const std::exception& e) {
            LOG_INFO() << "cannot connect to etcd. trying to reconnect: "
                       << e.what();
            sleep(1);
        }
    }
}
// wait the client ready
void wait_for_connection(etcd::SyncClient& client) {
    // wait until the client connects to etcd server
    while (!client.head().is_ok()) {
        sleep(1);
    }
}

// a loop for initialized a watcher with auto-restart capability
void initialize_watcher(etcd::SyncClient& client, const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher) {
    wait_for_connection(client);

    if (watcher && watcher->Cancelled()) {
        LOG_INFO() << "watcher's reconnect loop been cancelled";
        return;
    }
    watcher.reset(
        new etcd::Watcher(client, prefix, callback, true /*recursive*/));

    watcher->Wait([&client, prefix, callback,
                   &watcher](bool cancelled) mutable {
        if (cancelled) {
            LOG_INFO() << "watcher's reconnect loop stopped as been cancelled";
            return;
        }
        initialize_watcher(client, prefix, callback, watcher);
    });
}

} // namespace uh::cluster
