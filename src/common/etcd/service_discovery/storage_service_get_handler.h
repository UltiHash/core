
#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include "common/etcd/service_discovery/service_monitor.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/pointer_traits.h"
#include "storage_get_handler.h"

#include <iostream>
#include <ranges>
namespace uh::cluster {

struct storage_service_get_handler : public service_monitor<storage_interface>,
                                     public storage_get_handler {

    void add_client(size_t id,
                    const std::shared_ptr<storage_interface>& client) override {
        std::lock_guard l(m_mutex);
        m_clients.emplace(id, client);
        m_cv.notify_one();
    }
    void
    remove_client(size_t id,
                  const std::shared_ptr<storage_interface>& client) override {
        std::lock_guard l(m_mutex);
        m_clients.erase(id);
    }

    std::shared_ptr<storage_interface> get(const uint128_t& pointer) override {
        const auto id = pointer_traits::get_service_id(pointer);
        return get(id);
    }

    bool contains(std::size_t id) override {
        std::lock_guard l(m_mutex);
        return m_clients.contains(id);
    }

    std::vector<std::shared_ptr<storage_interface>> get_services() override {
        std::lock_guard l(m_mutex);
        std::vector<std::shared_ptr<storage_interface>> clients_list;
        clients_list.reserve(m_clients.size());

        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::map<std::size_t, std::shared_ptr<storage_interface>> m_clients;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICES_H
