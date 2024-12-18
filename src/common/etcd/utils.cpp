#include "utils.h"

#include "common/telemetry/log.h"

using namespace std::chrono_literals;

namespace uh::cluster {

std::unique_ptr<etcd::SyncClient>
make_etcd_client(const etcd_config& cfg = {}) {
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
                LOG_DEBUG() << "cannot connect to etcd. trying to reconnect";
                std::this_thread::sleep_for(1s);
                continue;
            }
            return client;
        } catch (const std::exception& e) {
            LOG_DEBUG() << "cannot connect to etcd. trying to reconnect: "
                        << e.what();
            std::this_thread::sleep_for(1s);
        }
    }
}

void wait_for_connection(etcd::SyncClient& client) {
    while (!client.head().is_ok()) {
        LOG_DEBUG() << "cannot connect to etcd. trying to reconnect";
        std::this_thread::sleep_for(1s);
    }
}

void initialize_watcher(std::unique_ptr<etcd::SyncClient>& client,
                        const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher) {
    client = make_etcd_client({});
    wait_for_connection(*client);

    if (watcher && watcher->Cancelled()) {
        LOG_INFO() << "watcher's reconnect loop been cancelled";
        return;
    }
    watcher.reset(
        new etcd::Watcher(*client, prefix, callback, true /*recursive*/));

    watcher->Wait([&client, prefix, callback,
                   &watcher](bool cancelled) mutable {
        if (cancelled) {
            LOG_INFO() << "watcher's reconnect loop stopped as been cancelled";
            return;
        }
        initialize_watcher(client, prefix, callback, watcher);
    });
}

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

void initialize_watcher(const etcd_config& cfg, const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher) {
    auto client = make_etcd_client(cfg);
    wait_for_connection(*client);

    if (watcher && watcher->Cancelled()) {
        return;
    }
    watcher.reset(
        new etcd::Watcher(*client, prefix, callback, true /*recursive*/));

    watcher->Wait([cfg, prefix, callback, &watcher](bool cancelled) mutable {
        if (cancelled) {
            return;
        }
        initialize_watcher(cfg, prefix, callback, watcher);
    });
}

// std::function<void(std::exception_ptr)>
// create_exception_handler(std::unique_ptr<etcd::SyncClient>& client,
//                          const etcd_config& cfg) {
//     static std::atomic<int> retry_count = 0;
//     static const int max_retries = 5;
//
//     return [&client, &cfg](std::exception_ptr eptr) {
//         try {
//             if (eptr) {
//                 std::rethrow_exception(eptr);
//             }
//         } catch (const std::runtime_error& e) {
//             std::cerr << "Runtime error in KeepAlive: " << e.what()
//                       << std::endl;
//
//             // Retry mechanism for short-term issues
//             if (retry_count < max_retries) {
//                 retry_count++;
//                 std::cerr << "Retrying (" << retry_count << "/" <<
//                 max_retries
//                           << ")..." << std::endl;
//                 std::this_thread::sleep_for(1s);
//             } else {
//                 std::cerr << "Max retries reached. Recreating etcd client."
//                           << std::endl;
//                 client = make_etcd_client(cfg); // Recreate etcd client
//                 retry_count = 0;                // Reset retry count
//             }
//
//         } catch (const std::out_of_range& e) {
//             std::cerr << "Lease expired: " << e.what() << std::endl;
//             // Lease expiry might require manual lease recreation
//         } catch (const std::exception& e) {
//             std::cerr << "Unexpected exception: " << e.what() << std::endl;
//         }
//     };
// }

} // namespace uh::cluster
