#pragma once

#include <etcd/KeepAlive.hpp>
#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
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
    struct response {
        const std::string& action;
        const std::string& key;
        const std::string& value;
    };
    using callback_t = std::function<void(response resp)>;
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
    void put_persistant(const std::string& key, const std::string& value);

    /*
     * Retrieve methods
     */
    std::string get(const std::string& key) const;
    bool has(const std::string& key) const;
    std::vector<std::string> keys(const std::string& prefix = "/") const;
    std::map<std::string, std::string>
    ls(const std::string& prefix = "/") const;

    /*
     * Remove methods
     */
    void rm(const std::string& key) noexcept;
    void rmdir(const std::string& prefix) noexcept;
    void clear_all() noexcept;

    class watch_guard {
    public:
        watch_guard() = default;
        watch_guard(watch_guard&&) = default;
        watch_guard& operator=(watch_guard&&) = default;

        ~watch_guard() {
            if (!m_prefix.empty())
                m_etcd->remove_watcher(m_prefix);
        }

    private:
        watch_guard(etcd_manager* etcd, const std::string& prefix,
                    callback_t callback)
            : m_etcd{etcd},
              m_prefix{prefix} {
            m_etcd->add_watcher(prefix, std::move(callback));
        }

        etcd_manager* m_etcd{nullptr};
        std::string m_prefix{};

        friend class etcd_manager;
    };

    /*
     * Watch given prefix "recursively"
     */
    [[nodiscard]] watch_guard watch(const std::string& prefix,
                                    callback_t callback) {
        return watch_guard(this, prefix, std::move(callback));
    }

    /*
     * Lock methods
     */
    class lock_guard {
    public:
        lock_guard() = default;
        lock_guard(lock_guard&&) = default;
        lock_guard& operator=(lock_guard&&) = default;

        ~lock_guard() {
            if (!m_unlock_key.empty())
                m_etcd->unlock(m_unlock_key);
        }

    private:
        lock_guard(etcd_manager* etcd, const std::string& lock_key)
            : m_etcd(etcd),
              m_unlock_key(m_etcd->lock(lock_key)) {}

        etcd_manager* m_etcd{nullptr};
        std::string m_unlock_key{};

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
        std::function<void(etcd::Response)> callback;
        std::unique_ptr<etcd::Watcher> watcher;

        watcher_entry() = default;
        watcher_entry(watcher_entry&&) = default;
        watcher_entry& operator=(watcher_entry&&) = default;
        watcher_entry(std::function<void(etcd::Response)> cb,
                      std::unique_ptr<etcd::Watcher> w)
            : callback(std::move(cb)),
              watcher(std::move(w)) {}
    };
    std::unordered_map<std::string, watcher_entry> m_watcher_entries;

    std::mutex m_mutex;

    void reset();

    void add_watcher(const std::string& prefix, callback_t callback);
    void remove_watcher(const std::string& prefix);
    void restore_watchers(void);

    std::string lock(const std::string& lock_key);
    void unlock(const std::string& unlock_key);
};

} // namespace uh::cluster
