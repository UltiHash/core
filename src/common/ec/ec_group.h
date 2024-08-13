//
// Created by massi on 8/13/24.
//
#ifndef EC_GROUP_H
#define EC_GROUP_H

#include "common/service_interfaces/storage_interface.h"
#include <shared_mutex>

namespace uh::cluster {

enum ec_status {
    degraded,
    healthy,
    recovering,
};

class ec_group {
    ec_status m_status = degraded;
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    const size_t m_id;

    void update_status() {

        for (const auto& n : m_nodes) {
            if (n == nullptr) {
                m_status = degraded;
                return;
            }
        }
        m_status = healthy;
    }

public:
    explicit ec_group(size_t gid, size_t data_nodes, size_t ec_nodes)
        : m_nodes(data_nodes + ec_nodes),
          m_id(gid) {}

    ec_group(ec_group&& eg) noexcept
        : m_status(eg.m_status),
          m_nodes(std::move(eg.m_nodes)),
          m_id(eg.m_id) {}

    void insert(size_t i, const std::shared_ptr<storage_interface>& node) {
        m_nodes.at(i) = node;
        update_status();
    }

    void remove(size_t i) {
        m_nodes.at(i) = nullptr;
        m_status = degraded;
    }

    [[nodiscard]] bool is_healthy() const noexcept {
        return m_status == healthy;
    }
};

} // namespace uh::cluster
#endif // EC_GROUP_H