#ifndef CORE_COMMON_ETCD_UTILS_H
#define CORE_COMMON_ETCD_UTILS_H

#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>
#include <memory>
#include <optional>
#include <string>

namespace uh::cluster {

struct etcd_config {
    // URL of the etcd service
    std::string url = "http://127.0.0.1:2379";

    std::optional<std::string> username;
    std::optional<std::string> password;
};

/**
 * Create etcd client
 */
std::unique_ptr<::etcd::SyncClient> make_etcd_client(const etcd_config& cfg);

/**
 * a loop for initialized a watcher with auto-restart capability
 */
void initialize_watcher(std::unique_ptr<etcd::SyncClient>& client,
                        const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher);
void initialize_watcher(etcd::SyncClient& client, const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher);
void initialize_watcher(const etcd_config& cfg, const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher);

// std::function<void(std::exception_ptr)>
// create_exception_handler(std::unique_ptr<etcd::SyncClient>& client,
//                          const etcd_config& cfg);

} // namespace uh::cluster

#endif
