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
    etcd_manager(const etcd_config& cfg)
        : m_cfg{cfg} {
        m_client.reset(create_client(m_cfg));
        m_lease = create_lease(*m_client);
        m_keepalive.reset(
            create_keepalive(*m_client, m_lease, get_exception_handler()));
    }

    /*
     * Save key value pair
     */
    void put(const std::string& key, const std::string& value) {
        m_key_value[key] = value;
        m_client->add(key, value, m_lease);
    }

    /*
     * Watch given path recursively
     */
    void watch(const std::string& path,
               std::function<void(etcd::Response)> callback) {
        watcher_entries.emplace_back(
            path, callback,
            std::make_unique<etcd::Watcher>(*m_client, path, callback, true));
    }

private:
    static constexpr int time_to_live = 30;
    const etcd_config& m_cfg;
    std::unique_ptr<etcd::SyncClient> m_client;

    int64_t m_lease;
    std::unique_ptr<etcd::KeepAlive> m_keepalive;
    std::map<std::string, std::string> m_key_value;

    struct watcher_entry {
        std::string path;
        std::function<void(etcd::Response)> callback;
        std::unique_ptr<etcd::Watcher> watcher;
    };
    std::vector<watcher_entry> watcher_entries;

    void restore_key_values(void) {
        for (const auto& [key, value] : m_key_value) {
            m_client->add(key, value, m_lease);
        }
    }

    void restore_watchers(void) {
        for (auto& e : watcher_entries) {
            e.watcher.reset(
                new etcd::Watcher(*m_client, e.path, e.callback, true));
        }
    }

    std::function<void(std::exception_ptr)> get_exception_handler() {
        return [this](std::exception_ptr eptr) {
            try {
                if (eptr) {
                    std::rethrow_exception(eptr);
                }
            } catch (const std::runtime_error& e) {
                LOG_DEBUG() << "cannot connect to etcd: " << e.what();
                m_client.reset(create_client(m_cfg));

                restore_watchers();

                m_lease = create_lease(*m_client);
                m_keepalive.reset(create_keepalive(*m_client, m_lease,
                                                   get_exception_handler()));

                restore_key_values();

            } catch (const std::out_of_range& e) {
                LOG_DEBUG() << "etcd lease expiry: " << e.what();
                m_lease = create_lease(*m_client);
                m_keepalive.reset(create_keepalive(*m_client, m_lease,
                                                   get_exception_handler()));

                restore_key_values();
            }
        };
    }

    /**************************************************************************
     * Static utilities
     */

    static void wait_for_connection(etcd::SyncClient& client) {
        using namespace std::chrono_literals;
        while (!client.head().is_ok()) {
            LOG_WARN() << "cannot connect to etcd. trying to reconnect";
            std::this_thread::sleep_for(1s);
        }
    }

    static etcd::SyncClient* create_client(const etcd_config& cfg) {
        etcd::SyncClient* client;
        if (cfg.username && cfg.password) {
            client =
                new etcd::SyncClient(cfg.url, *cfg.username, *cfg.password);
        } else {
            client = new etcd::SyncClient(cfg.url);
        }
        wait_for_connection(*client);
        return client;
    }

    static int64_t create_lease(etcd::SyncClient& client) {
        return client.leasegrant(time_to_live).value().lease();
    }
    static etcd::KeepAlive* create_keepalive(
        etcd::SyncClient& client, int64_t lease,
        std::function<void(std::exception_ptr)> exception_handler) {
        return new etcd::KeepAlive(client, exception_handler, time_to_live,
                                   lease);
    }
};

} // namespace uh::cluster

#endif
