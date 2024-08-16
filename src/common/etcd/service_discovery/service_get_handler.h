
#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include "common/etcd/service_discovery/service_monitor.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/pointer_traits.h"

#include <ranges>

namespace uh::cluster {

template <typename service_interface>
struct service_get_handler : public service_monitor<service_interface> {

    void add_client(size_t id,
                    const std::shared_ptr<service_interface>& client) override {
        std::lock_guard l(m_mutex);
        m_clients.emplace(id, client);
        m_cv.notify_one();
    }
    void
    remove_client(size_t id,
                  const std::shared_ptr<service_interface>& client) override {
        std::lock_guard l(m_mutex);
        m_clients.erase(id);
    }

    std::shared_ptr<service_interface> get(const uint128_t& pointer)
    requires std::is_same_v<service_interface, storage_interface>
    {
        const auto id = pointer_traits::get_service_id(pointer);
        return get(id);
    }

    std::shared_ptr<service_interface> get(std::size_t id) {
        std::shared_ptr<service_interface> cl;

        std::unique_lock lk(m_mutex);
        if (!m_cv.wait_for(lk, SERVICE_GET_TIMEOUT, [this, &id, &cl]() {
                if (auto v = m_clients.find(id); v != m_clients.cend()) {
                    cl = v->second;
                    return true;
                }
                return false;
            }))
            throw std::runtime_error(
                "timeout waiting for " +
                get_service_string(service_interface::service_role) +
                " client: " + std::to_string(id));

        return cl;
    }

    optref<const std::shared_ptr<service_interface>> at(std::size_t id) {
        std::lock_guard l(m_mutex);
        return m_clients.at(id);
    }

    std::vector<std::shared_ptr<service_interface>> get_services() {
        std::lock_guard l(m_mutex);
        std::vector<std::shared_ptr<service_interface>> clients_list;
        clients_list.reserve(m_clients.size());

        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::map<std::size_t, std::shared_ptr<service_interface>> m_clients;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICES_H
