#ifndef CORE_COMMON_ETCD_UTILS_H
#define CORE_COMMON_ETCD_UTILS_H

#include <etcd/KeepAlive.hpp>
#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

using namespace std::chrono_literals;
/**
 * This class handles all access to the etcd client, including error checking
 * and resetting the client.
 * Writing and reading should be managed in a single class, as every access
 * pattern should be reset when a problem occurs.
 */
class etcd_manager {
public:
    /*
     * Create etcd::SyncClient, lease, keepalive, and its exception handler to
     * detect connection failure.
     */
    explicit etcd_manager(const etcd_config& cfg = {}, int ttl = 30);
    ~etcd_manager();

    /*
     * Save key value pair
     */
    int put(const std::string& key, const std::string& value);

    etcd::Keys ls(const std::string& prefix = "/");

    void clear_all();

    /*
     * Watch given prefix recursively
     */
    void watch(const std::string& prefix,
               std::function<void(etcd::Response)> callback);

private:
    const etcd_config m_cfg;
    int m_ttl;
    std::unique_ptr<etcd::SyncClient> m_client;
    std::unique_ptr<etcd::Watcher> m_healthchecker;

    int64_t m_lease;
    std::unique_ptr<etcd::KeepAlive> m_keepalive;
    std::map<std::string, std::string> m_key_value;

    struct watcher_entry {
        std::string prefix;
        std::function<void(etcd::Response)> callback;
        std::unique_ptr<etcd::Watcher> watcher;
    };
    std::vector<watcher_entry> watcher_entries;

    std::mutex m_mutex;

    void reset();

    void restore_key_values(void);

    void restore_watchers(void);

    /**************************************************************************
     * Static utilities
     */
    static std::unique_ptr<etcd::SyncClient>
    create_client(const etcd_config& cfg = {});
};

} // namespace uh::cluster

#endif
