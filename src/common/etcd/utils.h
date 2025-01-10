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

using namespace std::chrono_literals;

/**
 * This class handles all access to the etcd client, including error checking
 * and resetting the client.
 * Writing and reading should be managed in a single class, as every access
 * pattern should be reset when a problem occurs.
 * This class does not save keys without lease.
 * And it clears all resources with revoking used lease.
 */
class etcd_manager {
public:
    /*
     * Create etcd::SyncClient, lease, keepalive, and its exception handler to
     * detect connection failure.
     */
    etcd_manager(const etcd_config& cfg = {}, int lease_timeout = 30);
    ~etcd_manager();

    /*
     * Save key value pair
     */
    void put(const std::string& key, const std::string& value);

    /*
     * Retrieve methods
     */
    std::string get(const std::string& key);
    bool has(const std::string& key);
    std::vector<std::string> keys(const std::string& prefix = "/");
    std::map<std::string, std::string> ls(const std::string& prefix = "/");

    /*
     * Remove methods
     */
    void rm(const std::string& key) noexcept;
    void rmdir(const std::string& prefix) noexcept;
    void clear_all() noexcept;

    /*
     * Watch given prefix recursively
     */
    void watch(const std::string& prefix,
               std::function<void(etcd::Response)> callback);

    /*
     * Lock methods
     */
    class lock_guard {
    public:
        explicit lock_guard(etcd_manager* etcd, const std::string& lock_key)
            : m_etcd(etcd),
              m_unlock_key(etcd->lock(lock_key)) {}

        ~lock_guard() { m_etcd->unlock(m_unlock_key); }

    private:
        etcd_manager* m_etcd;
        std::string m_unlock_key;

        friend class etcd_manager;
    };

    [[nodiscard]] lock_guard get_lock_guard(const std::string& lock_key) {
        return etcd_manager::lock_guard(this, lock_key);
    }

private:
    const etcd_config m_cfg;
    int m_lease_timeout;
    std::atomic<std::shared_ptr<etcd::SyncClient>> m_client;
    std::unique_ptr<etcd::Watcher> m_watchdog;

    int64_t m_lease;
    std::unique_ptr<etcd::KeepAlive> m_keepalive;

    struct watcher_entry {
        std::string prefix;
        std::function<void(etcd::Response)> callback;
        std::unique_ptr<etcd::Watcher> watcher;
    };
    std::vector<watcher_entry> watcher_entries;

    std::mutex m_mutex;

    void reset();

    void restore_watchers(void);

    std::string lock(const std::string& lock_key);
    void unlock(const std::string& unlock_key);
};

} // namespace uh::cluster

#endif
