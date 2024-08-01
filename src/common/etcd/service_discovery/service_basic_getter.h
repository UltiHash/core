
#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include "common/etcd/service_discovery/maintainer_monitor.h"
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/map_index.h"
#include "common/utils/pointer_traits.h"
#include <ranges>

namespace uh::cluster {

template <typename service_interface>
class service_basic_getter : public maintainer_monitor<service_interface> {
    void add_client(size_t id,
                    const std::shared_ptr<service_interface>& client) override {
        m_clients.add(id, client);
    }
    void
    remove_client(const std::shared_ptr<service_interface>& client) override {
        m_clients.remove(client);
    }

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

        std::unique_lock<std::mutex> lk(*this->m_mutex);
        if (this->m_cv->get().wait_for(lk,
                                       std::chrono::seconds(this->m_timeout_s),
                                       [this, &id, &cl]() {
                                           try {
                                               cl = m_clients.at(id);
                                           } catch (...) {
                                               return false;
                                           }
                                           return true;
                                       })) {
        } else
            throw std::runtime_error(
                "timeout waiting for " +
                get_service_string(service_interface::service_role) +
                " client: " + std::to_string(id));

        return cl;
    }

    std::vector<std::shared_ptr<service_interface>> get_services() const {

        std::vector<std::shared_ptr<service_interface>> clients_list;
        clients_list.reserve(m_clients.size());

        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    map_index<std::size_t, std::shared_ptr<service_interface>> m_clients;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICES_H
