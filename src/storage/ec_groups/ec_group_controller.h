#pragma once
#include "common/ec/ec_scheme.h"
#include "common/etcd/utils.h"
#include "common/service_interfaces/storage_interface.h"
#include "storage/interfaces/storage_group.h"

namespace uh::cluster {

struct ec_group_controller : public service_observer<storage_interface> {

public:
    ec_group_controller(boost::asio::io_context& ioc, size_t data_nodes,
                        size_t ec_nodes, etcd_manager& etcd,
                        bool active_recoverable_groups)
        : m_scheme(data_nodes, ec_nodes),
          m_ioc(ioc),
          m_etcd(etcd),
          m_active_recoverable_groups(active_recoverable_groups) {}

    void add_observer(service_observer<storage_group>& observer) {

        std::lock_guard l(m_mutex);
        for (const auto& [id, cl] : m_ec_groups) {
            observer.add_client(id, cl);
        }

        m_observers.emplace_back(observer);
    }

    void remove_observer(service_observer<storage_group>& observer) {
        m_observers.remove_if(
            [&observer](const service_observer<storage_group>& o) {
                return &observer == &o;
            });
    }

    void add_client(size_t id,
                    const std::shared_ptr<storage_interface>& cl) override {

        const auto gid = m_scheme.calc_group_id(id);
        const auto mid = m_scheme.calc_group_node_id(id);

        std::lock_guard l(m_mutex);

        auto it = m_ec_groups.find(gid);
        if (it == m_ec_groups.cend()) {
            it = m_ec_groups.emplace_hint(it, gid,
                                          std::make_shared<storage_group>(
                                              m_ioc, m_scheme.data_nodes(),
                                              m_scheme.ec_nodes(), gid, m_etcd,
                                              m_active_recoverable_groups));
        }
        it->second->insert(id, mid, cl);
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_interface>& cl) override {

        const auto gid = m_scheme.calc_group_id(id);
        const auto nid = m_scheme.calc_group_node_id(id);

        std::lock_guard l(m_mutex);

        if (const auto it = m_ec_groups.find(gid); it != m_ec_groups.cend()) {
            it->second->remove(id, nid);
        }
    }

private:
    std::mutex m_mutex;

    ec_scheme m_scheme;
    std::map<size_t, std::shared_ptr<storage_group>> m_ec_groups;
    std::list<std::reference_wrapper<service_observer<storage_group>>>
        m_observers;
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    const bool m_active_recoverable_groups;
};
} // namespace uh::cluster
