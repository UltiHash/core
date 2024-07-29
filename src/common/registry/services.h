
#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include "common/registry/namespace.h"
#include "common/utils/pointer_traits.h"
#include "service_maintainer.h"
#include <ranges>

namespace uh::cluster {

template <typename service_interface>
class services : service_maintainer<service_interface> {
    using service_maintainer<service_interface>::service_maintainer;
    using service_maintainer<service_interface>::m_mutex;
    using service_maintainer<service_interface>::m_clients;
    using service_maintainer<service_interface>::m_cv;
    using service_maintainer<service_interface>::m_local_service;
    using service_maintainer<service_interface>::m_load_balancer;

public:
    template <
        typename T = service_interface,
        typename = std::enable_if_t<std::is_same_v<T, storage_interface>, T>>
    std::shared_ptr<service_interface> get(const uint128_t& pointer) const {
        const auto id = pointer_traits::get_service_id(pointer);
        return get(id);
    }

    std::shared_ptr<service_interface> get(std::size_t id) const {
        std::shared_ptr<service_interface> cl;

        std::unique_lock<std::mutex> lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                          [this, &id, &cl]() {
                              auto it = m_clients.find(id);

                              if (it == m_clients.end())
                                  return false;

                              cl = it->second;
                              return true;
                          })) {
        } else
            throw std::runtime_error(
                "timeout waiting for " +
                get_service_string(service_interface::service_role) +
                " client: " + std::to_string(id));

        return cl;
    }

    std::shared_ptr<service_interface> get() const {
        std::unique_lock<std::mutex> lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                          [this]() { return !m_load_balancer.empty(); })) {
        } else
            throw std::runtime_error(
                "timeout waiting for any " +
                get_service_string(service_interface::service_role) +
                " client");

        if (m_local_service) {
            return m_local_service;
        }

        return m_load_balancer.get();
    }

    std::vector<std::shared_ptr<service_interface>> get_services() const {
        std::vector<std::shared_ptr<service_interface>> clients_list;

        std::lock_guard<std::mutex> lk(m_mutex);
        clients_list.reserve(m_clients.size());
        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    static constexpr std::size_t m_timeout_s = 10;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICES_H
