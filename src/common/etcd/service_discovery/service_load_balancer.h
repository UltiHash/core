
#ifndef UH_CLUSTER_STORAGE_LOAD_BALANCER_H
#define UH_CLUSTER_STORAGE_LOAD_BALANCER_H

#include "common/ec/ec_group.h"
#include "common/etcd/service_discovery/service_monitor.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/map_index.h"
#include "storage/interfaces/storage_system.h"

#include <set>

namespace uh::cluster {

template <typename service_interface>
struct load_balancer : service_monitor<service_interface> {
    [[nodiscard]] virtual std::shared_ptr<service_interface> get() const = 0;
    [[nodiscard]] virtual bool empty() const noexcept = 0;
};

class ec_load_balancer;

template <typename service_interface>
class roundrobin_load_balancer : public load_balancer<service_interface> {

    friend ec_load_balancer;
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

public:
    [[nodiscard]] std::shared_ptr<service_interface> get() const override {

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

    [[nodiscard]] bool empty() const noexcept override {
        return m_services.size() == 0;
    }

private:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;

    std::set<std::shared_ptr<service_interface>> m_services;
    mutable decltype(m_services.end()) m_robin_index = m_services.cend();
};

class capacity_load_balancer : public load_balancer<storage_interface> {

    void add_attribute(const std::shared_ptr<storage_interface>& client,
                       etcd_service_attributes attr,
                       const std::string& value) override {
        std::lock_guard l(m_mutex);

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
        std::lock_guard l(m_mutex);
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
    remove_client(size_t,
                  const std::shared_ptr<storage_interface>& client) override {
        m_free_spaces.remove(client);
        m_loads.remove(client);
    }

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;

    map_index<size_t, std::shared_ptr<storage_interface>> m_free_spaces;
    map_index<double, std::shared_ptr<storage_interface>> m_loads;

public:
    [[nodiscard]] std::shared_ptr<storage_interface> get() const override {

        std::unique_lock lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
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

    [[nodiscard]] bool empty() const noexcept override {
        return m_free_spaces.size() == 0 or m_loads.size() == 0;
    }
};

class ec_load_balancer : public load_balancer<storage_system> {

    void add_client(size_t id,
                    const std::shared_ptr<storage_system>& cl) override {
        if (cl->is_healthy()) {
            m_load_balancer.add_client(id, cl);
        }
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_system>& cl) override {
        m_load_balancer.remove_client(id, cl);
    }

    roundrobin_load_balancer<storage_system> m_load_balancer;

public:
    [[nodiscard]] std::shared_ptr<storage_system> get() const override {
        return m_load_balancer.get();
    }

    [[nodiscard]] bool empty() const noexcept override {
        return m_load_balancer.empty();
    }
};

} // namespace uh::cluster

#endif // UH_CLUSTER_STORAGE_LOAD_BALANCER_H
