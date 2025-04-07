#pragma once

#include "common/etcd/service_discovery/service_observer.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/pointer_traits.h"
#include "service_load_balancer.h"

#include <iostream>
#include <ranges>
namespace uh::cluster {

class storage_load_balancer : public service_load_balancer<storage_interface> {
public:
    explicit storage_load_balancer(
        std::chrono::milliseconds service_get_timeout = SERVICE_GET_TIMEOUT)
        : service_load_balancer<storage_interface>(service_get_timeout) {}

    using service_load_balancer<storage_interface>::get;

    virtual std::shared_ptr<storage_interface> get(const uint128_t& pointer) {
        const auto id = pointer_traits::get_service_id(pointer);
        return get(id);
    }

    virtual std::shared_ptr<storage_interface> get(std::size_t id) {
        std::shared_ptr<storage_interface> cl;

        std::unique_lock lk(m_mutex);
        if (!m_cv.wait_for(lk, m_service_get_timeout, [this, &id, &cl]() {
                if (auto v = m_services.find(id); v != m_services.cend()) {
                    cl = v->second;
                    return true;
                }
                return false;
            }))
            throw std::runtime_error(
                "timeout waiting for " +
                get_service_string(storage_interface::service_role) +
                " client: " + std::to_string(id));

        return cl;
    }

    bool contains(std::size_t id) {
        std::lock_guard l(m_mutex);
        return m_services.contains(id);
    }

    std::vector<std::shared_ptr<storage_interface>> get_services() {
        std::lock_guard l(m_mutex);
        std::vector<std::shared_ptr<storage_interface>> clients_list;
        clients_list.reserve(m_services.size());

        std::ranges::copy(m_services | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }
};

} // namespace uh::cluster
