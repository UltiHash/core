
#ifndef UH_CLUSTER_STORAGE_LOAD_BALANCER_H
#define UH_CLUSTER_STORAGE_LOAD_BALANCER_H

#include "common/utils/map_index.h"
#include "storage/interfaces/storage_interface.h"
#include <set>

namespace uh::cluster {

template <typename service_interface> struct service_load_balancer {

    void add_attribute(const std::shared_ptr<service_interface>& client,
                       etcd_service_attributes attr, const std::string& value) {
    }
    void remove_attribute(const std::shared_ptr<service_interface>& client,
                          etcd_service_attributes attr) {}
    void add_client(const std::shared_ptr<service_interface>& client) {
        m_services.emplace(client);
    }
    void remove_client(const std::shared_ptr<service_interface>& client) {

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

        if (m_robin_index == m_services.cend()) {
            m_robin_index = m_services.cbegin();
        }

        auto rv = *m_robin_index;
        ++m_robin_index;

        return rv;
    }

    [[nodiscard]] inline bool empty() const noexcept {
        return m_services.size() == 0;
    }

private:
    std::set<std::shared_ptr<service_interface>> m_services;
    mutable std::set<std::shared_ptr<service_interface>>::const_iterator
        m_robin_index = m_services.cend();
};

template <> struct service_load_balancer<storage_interface> {
    void add_client(const std::shared_ptr<storage_interface>&) {}

    void add_attribute(const std::shared_ptr<storage_interface>& client,
                       etcd_service_attributes attr, const std::string& value) {
        switch (attr) {
        case STORAGE_FREE_SPACE:
            m_free_spaces.add(std::stoul(value), client);
        case STORAGE_WRITE_DURATION:
            m_write_durations.add(std::stof(value), client);
        default:
            break;
        }
    }

    void remove_attribute(const std::shared_ptr<storage_interface>& client,
                          etcd_service_attributes attr) {
        switch (attr) {
        case STORAGE_FREE_SPACE:
            m_free_spaces.remove(client);
            break;
        case STORAGE_WRITE_DURATION:
            m_write_durations.remove(client);
            break;
        default:
            break;
        }
    }

    void remove_client(const std::shared_ptr<storage_interface>& client) {
        m_free_spaces.remove(client);
        m_write_durations.remove(client);
    }

    [[nodiscard]] inline bool empty() const noexcept {
        return m_free_spaces.size() == 0 or m_write_durations.size() == 0;
    }

    [[nodiscard]] std::shared_ptr<storage_interface> get() const {

        if (m_free_spaces.size() == 1) {
            return m_free_spaces.begin()->second;
        }

        auto candidate_dn = m_free_spaces.max();
        if (candidate_dn->second == m_write_durations.max()->second) {
            if (auto next_free_space = std::prev(candidate_dn)->first;
                next_free_space > EP_BUFFER) {
                candidate_dn--;
            }
        }

        return candidate_dn->second;
    }

    map_index<size_t, std::shared_ptr<storage_interface>> m_free_spaces;
    map_index<double, std::shared_ptr<storage_interface>> m_write_durations;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_STORAGE_LOAD_BALANCER_H
