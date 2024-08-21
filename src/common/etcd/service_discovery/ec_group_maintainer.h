#ifndef EC_GROUP_MAINTAINER_H
#define EC_GROUP_MAINTAINER_H
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/ec_scheme.h"
#include "service_monitor.h"
#include "storage/interfaces/storage_group.h"

namespace uh::cluster {

struct ec_group_maintainer : public service_monitor<storage_interface> {

    ec_group_maintainer(boost::asio::io_context& ioc, size_t data_nodes,
                        size_t ec_nodes)
        : m_scheme(data_nodes, ec_nodes),
          m_ioc(ioc) {}

    void add_monitor(service_monitor<storage_group>& monitor) {

        std::lock_guard l(m_mutex);
        for (const auto& [id, cl] : m_ec_groups) {
            monitor.add_client(id, cl);
        }

        m_monitors.emplace_back(monitor);
    }

    void remove_monitor(service_monitor<storage_group>& monitor) {
        m_monitors.remove_if(
            [&monitor](const service_monitor<storage_group>& m) {
                return &monitor == &m;
            });
    }

private:
    void add_client(size_t id,
                    const std::shared_ptr<storage_interface>& cl) override {
        const auto gid = m_scheme.calc_group_id(id);
        const auto nid = m_scheme.calc_group_node_id(id);

        std::lock_guard l(m_mutex);

        auto it = m_ec_groups.find(gid);
        if (it == m_ec_groups.cend()) {
            it = m_ec_groups.emplace_hint(
                it, gid,
                std::make_shared<storage_group>(m_ioc, m_scheme.data_nodes(),
                                                m_scheme.ec_nodes()));
        }
        it->second->insert(id, nid, cl);

        for (auto& m : m_monitors) {
            m.get().add_client(gid, it->second);
        }
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_interface>& cl) override {
        const auto gid = m_scheme.calc_group_id(id);
        const auto nid = m_scheme.calc_group_node_id(id);

        std::lock_guard l(m_mutex);

        if (const auto it = m_ec_groups.find(gid); it != m_ec_groups.cend()) {
            it->second->remove(id, nid);

            for (auto& m : m_monitors) {
                m.get().remove_client(gid, it->second);
            }
        }
    }

    std::mutex m_mutex;

    ec_scheme m_scheme;
    std::map<size_t, std::shared_ptr<storage_group>> m_ec_groups;
    std::list<std::reference_wrapper<service_monitor<storage_group>>>
        m_monitors;
    boost::asio::io_context& m_ioc;
};
} // namespace uh::cluster

#endif // EC_GROUP_MAINTAINER_H
