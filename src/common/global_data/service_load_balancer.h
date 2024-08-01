
#ifndef UH_CLUSTER_STORAGE_LOAD_BALANCER_H
#define UH_CLUSTER_STORAGE_LOAD_BALANCER_H

#include "common/registry/maintainer_monitor.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/map_index.h"
#include <set>

namespace uh::cluster {

template <typename service_interface>
struct service_load_balancer : public maintainer_monitor<service_interface> {

    void add_client(size_t,
                    const std::shared_ptr<service_interface>& client) override {
        m_services.emplace(client);
    }
    void
    remove_client(const std::shared_ptr<service_interface>& client) override {

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

    [[nodiscard]] std::shared_ptr<service_interface> get() const {

        if (this->m_local_service) {
            return this->m_local_service;
        }

        std::unique_lock<std::mutex> lk(*this->m_mutex);
        if (this->m_cv->get().wait_for(lk,
                                       std::chrono::seconds(this->m_timeout_s),
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
    std::set<std::shared_ptr<service_interface>> m_services;
    mutable decltype(m_services.end()) m_robin_index = m_services.cend();
};

template <>
struct service_load_balancer<storage_interface>
    : public maintainer_monitor<storage_interface> {

    void add_attribute(const std::shared_ptr<storage_interface>& client,
                       etcd_service_attributes attr,
                       const std::string& value) override {
        switch (attr) {
        case STORAGE_FREE_SPACE:
            m_free_spaces.add(std::stoul(value), client);
        case STORAGE_LOAD:
            m_loads.add(std::stof(value), client);
        default:
            break;
        }
    }

    void remove_attribute(const std::shared_ptr<storage_interface>& client,
                          etcd_service_attributes attr) override {
        switch (attr) {
        case STORAGE_FREE_SPACE:
            m_free_spaces.remove(client);
            break;
        case STORAGE_LOAD:
            m_loads.remove(client);
            break;
        default:
            break;
        }
    }

    void
    remove_client(const std::shared_ptr<storage_interface>& client) override {
        m_free_spaces.remove(client);
        m_loads.remove(client);
    }

    [[nodiscard]] inline bool empty() const noexcept {
        return m_free_spaces.size() == 0 or m_loads.size() == 0;
    }

    std::shared_ptr<storage_interface> get() const {

        std::unique_lock<std::mutex> lk(*this->m_mutex);
        if (this->m_cv->get().wait_for(lk, std::chrono::seconds(m_timeout_s),
                                       [this]() { return !empty(); })) {
        } else
            throw std::runtime_error("timeout waiting for any storages");

        auto candidate_dn = m_free_spaces.max();

        if (m_free_spaces.size() > 1) {

            if (candidate_dn->second == m_loads.max()->second) {
                if (const auto next_free_space = std::prev(candidate_dn)->first;
                    next_free_space >= INPUT_CHUNK_SIZE) {
                    --candidate_dn;
                }
            }
        }

        if (candidate_dn->first < INPUT_CHUNK_SIZE) {
            throw std::runtime_error("Load-balancer: Insufficient capacity");
        }
        return candidate_dn->second;
    }

    map_index<size_t, std::shared_ptr<storage_interface>> m_free_spaces;
    map_index<double, std::shared_ptr<storage_interface>> m_loads;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_STORAGE_LOAD_BALANCER_H
