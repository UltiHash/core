#pragma once

#include "common/etcd/service_discovery/service_monitor.h"
#include "common/utils/pointer_traits.h"
#include "storage/interfaces/distributed.h"
#include "storage_get_handler.h"

#include <ranges>

namespace uh::cluster {

struct storage_service_get_handler
    : public service_monitor<distributed_storage>,
      public storage_get_handler {

    storage_service_get_handler(
        std::chrono::milliseconds service_get_timeout = SERVICE_GET_TIMEOUT)
        : m_service_get_timeout{service_get_timeout} {}

    void
    add_client(size_t id,
               const std::shared_ptr<distributed_storage>& client) override {
        std::lock_guard l(m_mutex);
        m_clients.emplace(id, client);
        m_cv.notify_one();
    }

    void
    remove_client(size_t id,
                  const std::shared_ptr<distributed_storage>& client) override {
        std::lock_guard l(m_mutex);

        m_clients.erase(id);
    }

    std::shared_ptr<distributed_storage>
    get(const uint128_t& pointer) override {
        const auto id = pointer_traits::get_service_id(pointer);
        return get(id);
    }

    std::shared_ptr<distributed_storage> get(std::size_t id) override {
        std::shared_ptr<distributed_storage> cl;

        std::unique_lock lk(m_mutex);
        if (!m_cv.wait_for(lk, m_service_get_timeout, [this, &id, &cl]() {
                if (auto v = m_clients.find(id); v != m_clients.cend()) {
                    cl = v->second;
                    return true;
                }
                return false;
            }))
            throw std::runtime_error(
                "timeout waiting for " +
                get_service_string(distributed_storage::service_role) +
                " client: " + std::to_string(id));

        return cl;
    }

    bool contains(std::size_t id) override {
        std::lock_guard l(m_mutex);
        return m_clients.contains(id);
    }

    std::vector<std::shared_ptr<distributed_storage>> get_services() override {
        std::lock_guard l(m_mutex);
        std::vector<std::shared_ptr<distributed_storage>> clients_list;
        clients_list.reserve(m_clients.size());

        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

    std::vector<std::size_t> get_storage_ids() {
        std::lock_guard l(m_mutex);
        std::vector<size_t> ids;
        ids.reserve(m_clients.size());

        std::ranges::copy(m_clients | std::views::keys,
                          std::back_inserter(ids));
        return ids;
    }

    [[nodiscard]] size_t size() const noexcept { return m_clients.size(); }

private:
    std::chrono::milliseconds m_service_get_timeout;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::map<std::size_t, std::shared_ptr<distributed_storage>> m_clients;
};

} // namespace uh::cluster
