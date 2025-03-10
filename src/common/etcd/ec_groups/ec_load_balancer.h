#pragma once
#include "common/etcd/service_discovery/roundrobin_load_balancer.h"
#include "storage/interfaces/storage_group.h"
#include <chrono>

namespace uh::cluster {

struct ec_load_balancer : public service_monitor<storage_group> {
    ec_load_balancer(
        std::chrono::milliseconds service_get_timeout = SERVICE_GET_TIMEOUT)
        : m_service_get_timeout{service_get_timeout} {}

    void add_client(size_t,
                    const std::shared_ptr<storage_group>& client) override {

        std::lock_guard l(m_mutex);

        if (auto itr = std::ranges::find_if(m_services,
                                            [&client](auto cl) {
                                                return cl->group_id() ==
                                                       client->group_id();
                                            });
            itr == m_services.cend()) {
            m_services.emplace(client);
        }

        m_cv.notify_one();
    }

    void remove_client(size_t,
                       const std::shared_ptr<storage_group>& client) override {
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

    std::shared_ptr<storage_group> get() {

        std::unique_lock lk(m_mutex);

        itr_type cl;
        if (!m_cv.wait_for(lk, m_service_get_timeout, [this, &cl] {
                cl = get_next_healthy();
                return cl != m_services.cend();
            })) {
            throw std::runtime_error(
                "timeout waiting for any " +
                get_service_string(storage_group::service_role) + " client");
        }

        if (m_robin_index == m_services.cend()) {
            m_robin_index = m_services.cbegin();
        }

        ++m_robin_index;
        return *cl;
    }

    [[nodiscard]] bool empty() const noexcept { return m_services.empty(); }

    [[nodiscard]] size_t size() const noexcept { return m_services.size(); }

private:
    std::chrono::milliseconds m_service_get_timeout;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::set<std::shared_ptr<storage_group>> m_services;
    using itr_type =
        typename std::set<std::shared_ptr<storage_group>>::const_iterator;
    itr_type m_robin_index = m_services.cend();

    [[nodiscard]] itr_type get_next_healthy() const noexcept {
        auto cl = std::find_if(m_robin_index, m_services.cend(),
                               [](auto cl) { return cl->is_healthy(); });
        if (cl == m_services.cend()) {
            cl = std::find_if(m_services.cbegin(), m_robin_index,
                              [](auto cl) { return cl->is_healthy(); });
        }
        return cl;
    }
};

} // namespace uh::cluster
