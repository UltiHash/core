
#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include "common/registry/namespace.h"
#include "common/registry/service_id.h"
#include "common/utils/pointer_traits.h"
#include "common/utils/service_factory.h"
#include "common/utils/time_utils.h"
#include "service_maintainer.h"
#include "storage/interfaces/storage_interface.h"
#include "third-party/etcd-cpp-apiv3/etcd/SyncClient.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/Watcher.hpp"
#include <ranges>

namespace uh::cluster {


template <typename service_interface>
class services: service_maintainer<service_interface> {
    using service_maintainer<service_interface>::service_maintainer;
    using service_maintainer<service_interface>::m_mutex;
    using service_maintainer<service_interface>::m_clients;
    using service_maintainer<service_interface>::m_cv;
    using service_maintainer<service_interface>::m_local_service;
    using service_maintainer<service_interface>::m_robin_index;

public:

    template <typename T = service_interface, typename = std::enable_if_t<std::is_same_v<T, storage_interface>, T>>
    std::shared_ptr<service_interface> get(const uint128_t& pointer) const {
        std::shared_ptr<service_interface> cl;

        const auto id = pointer_traits::get_service_id(pointer);
        return get(id);
    }

    std::shared_ptr<service_interface> get(std::size_t id) const {
        std::shared_ptr<service_interface> cl;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
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

    std::shared_ptr<service_interface> get() const;

    std::vector<std::shared_ptr<service_interface>> get_services() const {
        std::vector<std::shared_ptr<service_interface>> clients_list;

        std::shared_lock<std::shared_mutex> lk(m_mutex);
        clients_list.reserve(m_clients.size());
        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    static constexpr std::size_t m_timeout_s = 10;

};

template <typename service_interface>
inline std::shared_ptr<service_interface> services<service_interface>::get() const {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                      [this]() { return !m_clients.empty(); })) {
    } else
        throw std::runtime_error(
            "timeout waiting for any " +
            get_service_string(service_interface::service_role) +
            " client");

    if (m_local_service) {
        return m_local_service;
    }

    if (m_robin_index == m_clients.end()) {
        m_robin_index = m_clients.begin();
    }

    auto rv = m_robin_index->second;
    ++m_robin_index;

    return rv;
}

template <>
inline std::shared_ptr<storage_interface> services<storage_interface>::get() const {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                      [this]() { return !m_clients.empty(); })) {
    } else
        throw std::runtime_error(
            "timeout waiting for any " +
            get_service_string(storage_interface::service_role) +
            " client");

    if (auto lc = m_service_factory.get_local_service(); lc) {
        return lc;
    }


    if (m_robin_index == m_clients.end()) {
        m_robin_index = m_clients.begin();
    }

    auto& se = m_detected_service_endpoints.at(m_robin_index->first);
    auto free_space = std::stoul(se.attributes.at(STORAGE_FREE_SPACE));
    auto write_duration = std::stof(se.attributes.at(STORAGE_WRITE_DURATION));

    double probability = free_space / write_duration;

    auto rv = m_robin_index->second;
    ++m_robin_index;

    return rv;
}

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICES_H
