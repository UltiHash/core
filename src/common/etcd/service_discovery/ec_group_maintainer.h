//
// Created by massi on 8/13/24.
//

#ifndef EC_GROUP_MAINTAINER_H
#define EC_GROUP_MAINTAINER_H
#include "../../utils/ec_scheme.h"
#include "common/service_interfaces/storage_interface.h"
#include "service_monitor.h"
#include "storage/interfaces/storage_system.h"

namespace uh::cluster {

class ec_group_maintainer : public service_monitor<storage_interface> {

    void add_client(size_t id,
                    const std::shared_ptr<storage_interface>& cl) override {
        const auto gid = m_scheme.get_group_id(id);
        const auto nid = m_scheme.get_group_node_id(id);

        std::lock_guard l(m_mutex);

        auto it = m_ec_groups.find(gid);
        if (it == m_ec_groups.cend()) {
            it = m_ec_groups.emplace_hint(
                it, gid,
                std::make_shared<storage_system>(m_ioc, m_scheme.data_nodes(),
                                                 m_scheme.ec_nodes()));
        }
        it->second->insert(id, nid, cl);

        for (auto& m : m_monitors) {
            m.get().add_client(gid, it->second);
        }
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_interface>& cl) override {
        const auto gid = m_scheme.get_group_id(id);
        const auto nid = m_scheme.get_group_node_id(id);

        std::lock_guard l(m_mutex);

        if (const auto it = m_ec_groups.find(gid); it != m_ec_groups.cend()) {
            it->second->remove(id, nid);

            for (auto& m : m_monitors) {
                m.get().remove_client(gid, it->second);
            }
        }
    }

    mutable std::mutex m_mutex;

    ec_scheme m_scheme;
    std::map<size_t, std::shared_ptr<storage_system>> m_ec_groups;
    std::list<std::reference_wrapper<service_monitor<storage_system>>>
        m_monitors;
    boost::asio::io_context& m_ioc;

public:
    explicit ec_group_maintainer(boost::asio::io_context& ioc,
                                 size_t data_nodes, size_t ec_nodes)
        : m_scheme(data_nodes, ec_nodes),
          m_ioc(ioc) {}

    void add_monitor(service_monitor<storage_system>& monitor) {

        std::lock_guard l(m_mutex);
        for (const auto& [id, cl] : m_ec_groups) {
            monitor.add_client(id, cl);
        }

        m_monitors.emplace_back(monitor);
    }
};
} // namespace uh::cluster

#endif // EC_GROUP_MAINTAINER_H
