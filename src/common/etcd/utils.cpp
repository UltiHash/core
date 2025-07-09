#include "utils.h"

#include "namespace.h"
#include <common/telemetry/log.h>
#include <format>
#include <stdexcept>

using namespace std::chrono_literals;

namespace uh::cluster {

etcd_manager::etcd_manager(const etcd_config& cfg, int lease_timeout)
    : m_cfg{cfg},
      m_lease_timeout{lease_timeout} {
    if (m_lease_timeout < 2) {
        throw std::runtime_error("ttl(" + std::to_string(m_lease_timeout) +
                                 ") should be bigger than 2, to make sure "
                                 "keepalive is smaller than lease time");
    }
    reset();
}

/*
 * Private utilities
 */
namespace {
std::shared_ptr<etcd::SyncClient> create_client(const etcd_config& cfg) {
    while (true) {
        try {
            std::shared_ptr<etcd::SyncClient> client;
            if (cfg.username && cfg.password) {
                client = std::make_unique<etcd::SyncClient>(
                    cfg.url, *cfg.username, *cfg.password);
            } else {
                client = std::make_unique<etcd::SyncClient>(cfg.url);
            }

            if (!client->head().is_ok()) {
                LOG_DEBUG() << "cannot connect to etcd. trying to reconnect: "
                            << client->head().error_code();
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
} // namespace

void etcd_manager::reset() {
    {
        auto client = create_client(m_cfg);

        auto lease_result = client->leasegrant(m_lease_timeout);
        if (!lease_result.is_ok()) {
            throw std::runtime_error("Failed to grant lease");
        }
        m_lease = lease_result.value().lease();

        m_keepalive.reset(
            new etcd::KeepAlive(*client, m_lease_timeout / 2, m_lease));

        m_watchdog.reset(new etcd::Watcher(*client, etcd_watchdog, {}, false));

        m_client.store(client);

        restore_watchers();
    }
    m_watchdog->Wait([this](bool cancelled) mutable {
        if (cancelled) {
            return;
        }
        reset();
    });
}

etcd_manager::~etcd_manager() {
    auto client = m_client.load();
    m_watchdog->Cancel();
    for (auto& [k, e] : m_watcher_entries) {
        e.watcher->Cancel();
    }
    client->leaserevoke(m_lease);
}

void etcd_manager::put(const std::string& key, const std::string& value) {
    auto client = m_client.load();

    auto resp = client->put(key, value, m_lease);
    if (!resp.is_ok())
        throw std::invalid_argument(
            "setting configuration parameter " + key +
            " failed, details: " + resp.error_message());
}

void etcd_manager::put_persistant(const std::string& key,
                                  const std::string& value) {
    auto client = m_client.load();

    auto resp = client->put(key, value);
    if (!resp.is_ok())
        throw std::invalid_argument(
            "setting configuration parameter " + key +
            " failed, details: " + resp.error_message());
}

etcd::Response etcd_manager::create_if_empty(std::string const& key,
                                             std::string const& value) {

    auto client = m_client.load();
    auto resp = client->modify_if(key, value, 0, m_lease);
    return resp;
}

std::string etcd_manager::get(const std::string& key) const {
    auto client = m_client.load();
    auto resp = client->get(key);
    if (!resp.is_ok())
        return "";
    return resp.value().as_string();
}

bool etcd_manager::has(const std::string& key) const {
    auto client = m_client.load();
    auto resp = client->get(key);
    return resp.is_ok();
}

std::vector<std::string> etcd_manager::keys(const std::string& prefix) const {
    auto client = m_client.load();
    return client->keys(prefix).keys();
}

std::map<std::string, std::string>
etcd_manager::ls(const std::string& prefix) const {
    auto client = m_client.load();
    auto resp = client->ls(prefix);
    auto keys = resp.keys();
    std::map<std::string, std::string> ret;
    for (auto i = 0u; i < keys.size(); ++i) {
        ret[keys[i]] = resp.value(i).as_string();
    }
    return ret;
}

int64_t etcd_manager::ls(const std::string& prefix, callback_t callback) const {

    auto client = m_client.load();
    auto resp = client->ls(prefix);
    if (resp.is_ok()) {
        auto values = resp.values();
        for (auto i = 0u; i < values.size(); ++i) {
            auto val = values[i];
            callback(response(resp.action(), val.key(), val.as_string()));
        }
    }
    return resp.index();
}

void etcd_manager::rm(const std::string& key) noexcept {
    auto client = m_client.load();
    client->rm(key);
}

void etcd_manager::rmdir(const std::string& prefix) noexcept {
    auto client = m_client.load();
    client->rmdir(prefix, true);
}

void etcd_manager::clear_all() noexcept { rmdir("/"); }

void etcd_manager::add_watcher(const std::string& prefix, callback_t callback,
                               int64_t watch_index) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto client = m_client.load();

    if (m_watcher_entries.contains(prefix)) {
        LOG_FATAL() << "watcher for prefix " << prefix << " already exists";
        throw std::invalid_argument("watcher for prefix " + prefix +
                                    " already exists");
    }

    if (watch_index == 0) {
        watch_index = ls(prefix, callback) + 1;
    }

    auto wrapper = [cb = std::move(callback)](const etcd::Response& resp) {
        if (resp.is_ok()) {
            auto values = resp.values();
            auto actions = resp.actions();
            for (auto i = 0u; i < values.size(); ++i) {
                auto action = actions[i];
                auto val = values[i];
                cb(response(action, val.key(), val.as_string()));
            }
        }
    };
    m_watcher_entries[prefix] = watcher_entry(
        wrapper, std::make_unique<etcd::Watcher>(*client, prefix, watch_index,
                                                 std::move(wrapper), true));
}

void etcd_manager::remove_watcher(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_watcher_entries.find(prefix);
    if (it == m_watcher_entries.end()) {
        LOG_FATAL() << "watcher for prefix " << prefix << " does not exist";
        throw std::invalid_argument("watcher for prefix " + prefix +
                                    " does not exist");
    }

    it->second.watcher->Cancel();

    if (m_watcher_entries.erase(prefix) == 0) {
        throw std::invalid_argument("watcher for prefix " + prefix +
                                    " does not exist");
    }
}

std::string etcd_manager::lock(const std::string& lock_key) {
    auto client = m_client.load();

    auto resp = client->lock_with_lease(lock_key, m_lease);
    if (!resp.is_ok())
        throw std::invalid_argument(
            "getting lock with lock_key " + lock_key +
            " failed, details: " + resp.error_message());
    return resp.lock_key();
}

void etcd_manager::unlock(const std::string& unlock_key) {
    auto client = m_client.load();
    auto resp = client->unlock(unlock_key);
    if (!resp.is_ok())
        throw std::invalid_argument(
            "releasing lock with unlock_key " + unlock_key +
            " failed, details: " + resp.error_message());
}

void etcd_manager::restore_watchers(void) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto client = m_client.load();

    for (auto& [k, e] : m_watcher_entries) {
        e.watcher.reset(new etcd::Watcher(*client, k, e.callback, true));
    }
}

} // namespace uh::cluster
