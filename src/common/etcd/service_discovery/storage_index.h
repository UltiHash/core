#pragma once

#include "common/etcd/service_discovery/service_observer.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/pointer_traits.h"
#include "service_load_balancer.h"

#include <ranges>

namespace uh::cluster {

class storage_index : public service_observer<storage_interface> {
public:
    explicit storage_index(
        std::chrono::milliseconds service_get_timeout = SERVICE_GET_TIMEOUT)
        : m_service_get_timeout{service_get_timeout} {}

    void add_client(size_t id,
                    std::shared_ptr<storage_interface> service) override {
        std::lock_guard l(m_mutex);
        m_services.emplace(id, service);
        m_cv.notify_one();
    }
    void remove_client(size_t id) override {
        std::lock_guard l(m_mutex);
        m_services.erase(id);
    }

    std::shared_ptr<storage_interface> get(const uint128_t& pointer) {
        const auto id = pointer_traits::get_service_id(pointer);
        std::shared_ptr<storage_interface> rv;

        std::unique_lock lk(m_mutex);
        if (!m_cv.wait_for(lk, m_service_get_timeout, [this, &id, &rv]() {
                if (auto it = m_services.find(id); it != m_services.cend()) {
                    rv = it->second;
                    return true;
                }
                return false;
            }))
            throw std::runtime_error(
                "timeout waiting for " +
                get_service_string(storage_interface::service_role) +
                " client: " + std::to_string(id));

        return rv;
    }

    std::vector<std::shared_ptr<storage_interface>> get_services() {
        std::lock_guard l(m_mutex);
        std::vector<std::shared_ptr<storage_interface>> clients_list;
        clients_list.reserve(m_services.size());

        std::ranges::copy(m_services | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    std::chrono::milliseconds m_service_get_timeout;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::map<std::size_t, std::shared_ptr<storage_interface>> m_services;
};

} // namespace uh::cluster
