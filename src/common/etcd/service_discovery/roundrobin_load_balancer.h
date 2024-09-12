
#ifndef UH_CLUSTER_STORAGE_LOAD_BALANCER_H
#define UH_CLUSTER_STORAGE_LOAD_BALANCER_H

#include "common/etcd/service_discovery/service_monitor.h"
#include "storage/interfaces/storage_group.h"

#include <set>

namespace uh::cluster {

template <typename service_interface>
struct roundrobin_load_balancer : public service_monitor<service_interface> {

    void add_client(size_t,
                    const std::shared_ptr<service_interface>& client) override {
        std::lock_guard l(m_mutex);

        m_services.emplace(client);
        m_cv.notify_one();
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

    std::shared_ptr<service_interface> get() {

        std::unique_lock lk(m_mutex);

        if (!m_cv.wait_for(lk, SERVICE_GET_TIMEOUT,
                           [this] { return !empty(); })) {
            throw std::runtime_error(
                "timeout waiting for any " +
                get_service_string(service_interface::service_role) +
                " client");
        }

        if (m_robin_index == m_services.cend()) {
            m_robin_index = m_services.cbegin();
        }

        auto rv = *m_robin_index;
        ++m_robin_index;

        return rv;
    }

    [[nodiscard]] bool empty() const noexcept { return m_services.empty(); }

    [[nodiscard]] size_t size() const noexcept { return m_services.size(); }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::set<std::shared_ptr<service_interface>> m_services;
    typename std::set<std::shared_ptr<service_interface>>::const_iterator
        m_robin_index = m_services.cend();
};

} // namespace uh::cluster

#endif // UH_CLUSTER_STORAGE_LOAD_BALANCER_H
