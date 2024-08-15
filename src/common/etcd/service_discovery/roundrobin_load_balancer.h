
#ifndef UH_CLUSTER_STORAGE_LOAD_BALANCER_H
#define UH_CLUSTER_STORAGE_LOAD_BALANCER_H

#include "common/etcd/service_discovery/service_monitor.h"
#include "storage/interfaces/storage_system.h"

#include <set>

namespace uh::cluster {

template <typename service_interface>
struct roundrobin_load_balancer : public service_monitor<service_interface> {

    void add_client(size_t,
                    const std::shared_ptr<service_interface>& client) override {
        std::lock_guard l(m_mutex);

        m_services.emplace(client);
    }
    void
    remove_client(size_t,
                  const std::shared_ptr<service_interface>& client) override {
        std::lock_guard l(m_mutex);

        auto it = m_services.find(client);
        if (it == m_services.end()) {
            return;
        }
        if (it == m_robin_index) {
            m_robin_index = m_services.erase(it);
        } else {
            m_services.erase(it);
        }
    }

    std::shared_ptr<service_interface> get() const {

        if (this->m_local_service) {
            return this->m_local_service;
        }

        std::unique_lock lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(this->m_timeout_s),
                          [this]() { return !empty(); })) {
        } else
            throw std::runtime_error(
                "timeout waiting for any " +
                get_service_string(service_interface::service_role) +
                " client");

        if (m_robin_index == m_services.cend()) {
            m_robin_index = m_services.cbegin();
        }

        auto rv = *m_robin_index;
        ++m_robin_index;

        return rv;
    }

    [[nodiscard]] bool empty() const noexcept { return m_services.size() == 0; }

private:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;

    std::set<std::shared_ptr<service_interface>> m_services;
    mutable decltype(m_services.end()) m_robin_index = m_services.cend();
};

} // namespace uh::cluster

#endif // UH_CLUSTER_STORAGE_LOAD_BALANCER_H
