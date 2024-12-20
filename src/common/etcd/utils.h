#ifndef CORE_COMMON_ETCD_UTILS_H
#define CORE_COMMON_ETCD_UTILS_H

#include "common/telemetry/log.h"

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
    static std::shared_ptr<etcd_manager> create(const etcd_config& cfg = {},
                                                int ttl = 30) {
        auto instance = std::make_shared<etcd_manager>(cfg, ttl);
        reset(instance);
        return instance;
    }
    explicit etcd_manager(const etcd_config& cfg, int ttl)
        : m_cfg{cfg},
          m_ttl{ttl} {

        m_client = create_client(m_cfg);
    }
    ~etcd_manager() {
        m_healthchecker->Cancel();
        for (auto& e : watcher_entries) {
            e.watcher->Cancel();
        }
    }

    /*
     * Save key value pair
     */
    void add(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_key_value[key] = value;
        m_client->add(key, value, m_lease);
    }

    etcd::Keys ls(const std::string& prefix = "/") {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_client->ls(prefix).keys();
    }

    void clear_all() {
        LOG_DEBUG() << "etcd_manager.clear_all() called";
        std::lock_guard<std::mutex> lock(m_mutex);
        m_key_value.clear();
        for (const auto& key : ls()) {
            m_client->rm(key);
        }
    }

    /*
     * Watch given prefix recursively
     */
    void watch(const std::string& prefix,
               std::function<void(etcd::Response)> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        watcher_entries.emplace_back(
            prefix, callback,
            std::make_unique<etcd::Watcher>(*m_client, prefix, callback, true));
    }

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

    void restore_key_values(void) {
        for (const auto& [key, value] : m_key_value) {
            m_client->put(key, value, m_lease);
        }
    }

    void restore_watchers(void) {
        for (auto& e : watcher_entries) {
            e.watcher.reset(
                new etcd::Watcher(*m_client, e.prefix, e.callback, true));
        }
    }

    /**************************************************************************
     * Static utilities
     */
    static std::unique_ptr<etcd::SyncClient>
    create_client(const etcd_config& cfg = {}) {
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
                    LOG_DEBUG()
                        << "cannot connect to etcd. trying to reconnect";
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

    static void reset(std::shared_ptr<etcd_manager>& t) {
        {
            std::lock_guard<std::mutex> lock(t->m_mutex);
            // Recreate etcd client to recover quickly (15s -> 1s)
            if (t->m_client && t->m_healthchecker &&
                t->m_healthchecker->Cancelled()) {
                return;
            }

            LOG_DEBUG() << "etcd_manager.reset() called";
            LOG_DEBUG() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@";
            LOG_DEBUG() << t->m_cfg.url;
            LOG_DEBUG() << "###########################";

            t->m_client = create_client(t->m_cfg);

            // Initialization
            t->m_lease = t->m_client->leasegrant(t->m_ttl).value().lease();
            t->m_keepalive.reset(
                new etcd::KeepAlive(*t->m_client, t->m_ttl / 2, t->m_lease));
            t->restore_key_values();
            t->restore_watchers();

            static constexpr const char* etcd_healthcheck = "/uh/healthcheck/";
            t->m_healthchecker.reset(
                new etcd::Watcher(*t->m_client, etcd_healthcheck, {}, false));
        }
        t->m_healthchecker->Wait([&t](bool cancelled) mutable {
            if (cancelled) {
                return;
            }
            reset(t);
        });
    }
};

} // namespace uh::cluster

#endif
