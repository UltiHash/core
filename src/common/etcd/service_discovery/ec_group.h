//
// Created by massi on 8/1/24.
//

#ifndef EC_GROUP_H
#define EC_GROUP_H
#include "common/service_interfaces/storage_interface.h"
#include "maintainer_monitor.h"

namespace uh::cluster {

enum ec_status {
    degraded,
    healthy,
    recovering,
};

class ec_group : maintainer_monitor<storage_interface> {

    void add_client(size_t id,
                    const std::shared_ptr<storage_interface>& cl) override {
        if (get_group_id(id) == m_group_id) {
            m_nodes[get_node_id(id)] = cl;
            for (auto& node : m_nodes) {
                if (node == nullptr) {
                    return;
                }
            }
            m_status = healthy;
        }
    }
    void remove_client(const std::shared_ptr<storage_interface>& cl) override {
        for (auto& node : m_nodes) {
            if (node == cl) {
                node.reset();
                m_status = degraded;
                break;
            }
        }
    }

    [[nodiscard]] size_t get_group_id(size_t node_id) const {
        return node_id / (m_data_nodes + m_ec_nodes);
    }

    [[nodiscard]] size_t get_node_id(size_t node_id) const {
        return node_id % (m_data_nodes + m_ec_nodes);
    }

    const size_t m_group_id;
    const size_t m_data_nodes;
    const size_t m_ec_nodes;
    ec_status m_status = degraded;
    std::vector<std::shared_ptr<storage_interface>> m_nodes;

public:
    ec_group(size_t group_id, size_t data_nodes, size_t ec_nodes)
        : m_group_id(group_id),
          m_data_nodes(data_nodes),
          m_ec_nodes(ec_nodes),
          m_nodes(m_data_nodes + m_ec_nodes) {}
};

} // namespace uh::cluster

#endif // EC_GROUP_H
