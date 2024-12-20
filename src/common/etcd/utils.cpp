#include "utils.h"

#include "common/telemetry/log.h"
#include "namespace.h"

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

etcd_manager::etcd_manager(const etcd_config& cfg, int ttl)
    : m_cfg{cfg},
      m_ttl{ttl} {

    m_client = create_client(m_cfg);
    reset();
}

etcd_manager::~etcd_manager() {
    m_healthchecker->Cancel();
    for (auto& e : watcher_entries) {
        e.watcher->Cancel();
    }
}

/*
 * Save key value pair
 */
int etcd_manager::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_key_value[key] = value;
    auto resp = m_client->put(key, value, m_lease);
    return resp.error_code();
}

etcd::Keys etcd_manager::ls(const std::string& prefix) {
    return m_client->ls(prefix).keys();
}

void etcd_manager::clear_all() {
    LOG_DEBUG() << "etcd_manager.clear_all() called";
    m_key_value.clear();
    for (const auto& key : ls()) {
        m_client->rm(key);
    }
}

void etcd_manager::watch(const std::string& prefix,
                         std::function<void(etcd::Response)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    watcher_entries.emplace_back(
        prefix, callback,
        std::make_unique<etcd::Watcher>(*m_client, prefix, callback, true));
}

void etcd_manager::reset() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Recreate etcd client to recover quickly (15s -> 1s)
        if (m_client && m_healthchecker && m_healthchecker->Cancelled()) {
            return;
        }

        m_client = create_client(m_cfg);

        // Initialization
        m_lease = m_client->leasegrant(m_ttl).value().lease();
        m_keepalive.reset(new etcd::KeepAlive(*m_client, m_ttl / 2, m_lease));
        restore_key_values();
        restore_watchers();

        m_healthchecker.reset(
            new etcd::Watcher(*m_client, etcd_healthcheck, {}, false));
    }
    m_healthchecker->Wait([this](bool cancelled) mutable {
        if (cancelled) {
            return;
        }
        reset();
    });
}

void etcd_manager::restore_key_values(void) {
    for (const auto& [key, value] : m_key_value) {
        m_client->put(key, value, m_lease);
    }
}

void etcd_manager::restore_watchers(void) {
    for (auto& e : watcher_entries) {
        e.watcher.reset(
            new etcd::Watcher(*m_client, e.prefix, e.callback, true));
    }
}

/**************************************************************************
 * Static utilities
 */
std::unique_ptr<etcd::SyncClient>
etcd_manager::create_client(const etcd_config& cfg) {
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

} // namespace uh::cluster
