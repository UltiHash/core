//
// Created by massi on 8/13/24.
//

#ifndef EC_GROUP_MAINTAINER_H
#define EC_GROUP_MAINTAINER_H
#include "common/service_interfaces/storage_interface.h"
#include "service_monitor.h"
#include "storage/interfaces/storage_system.h"

namespace uh::cluster {

class ec_group_maintainer : service_monitor<storage_interface> {

    void add_client(size_t id,
                    const std::shared_ptr<storage_interface>& cl) override {
        const auto gid = get_group_id(id);
        const auto nid = get_node_id(id);

        auto it = m_ec_groups.find(gid);
        if (it == m_ec_groups.cend()) {
            it = m_ec_groups.emplace_hint(it, gid,
                                          std::make_shared<storage_system>(
                                              gid, m_data_nodes, m_ec_nodes));
        }
        it->second->get_group().insert(nid, cl);
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_interface>& cl) override {
        const auto gid = get_group_id(id);
        const auto nid = get_node_id(id);

        if (const auto it = m_ec_groups.find(gid); it != m_ec_groups.cend()) {
            it->second->get_group().remove(nid);
        }
    }

    [[nodiscard]] size_t get_group_id(size_t node_id) const {
        return node_id / (m_data_nodes + m_ec_nodes);
    }

    [[nodiscard]] size_t get_node_id(size_t node_id) const {
        return node_id % (m_data_nodes + m_ec_nodes);
    }

    const size_t m_data_nodes;
    const size_t m_ec_nodes;

    std::map<size_t, std::shared_ptr<storage_system>> m_ec_groups;
    std::list<std::reference_wrapper<service_monitor<storage_system>>>
        m_monitors;

public:
    explicit ec_group_maintainer(size_t data_nodes, size_t ec_nodes)
        : m_data_nodes(data_nodes),
          m_ec_nodes(ec_nodes) {}
};
} // namespace uh::cluster

#endif // EC_GROUP_MAINTAINER_H
