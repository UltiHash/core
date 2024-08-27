
#ifndef EC_SCHEME_H
#define EC_SCHEME_H
#include "common/types/big_int.h"
#include "common/utils/pointer_traits.h"

namespace uh::cluster {
struct ec_scheme {

    ec_scheme(size_t data_nodes, size_t ec_nodes)
        : m_data_nodes(data_nodes),
          m_ec_nodes(ec_nodes),
          m_group_size(m_data_nodes + m_ec_nodes) {}

    [[nodiscard]] size_t calc_group_id(size_t node_id) const {
        return node_id / m_group_size;
    }

    [[nodiscard]] size_t calc_group_id(const uint128_t& global_pointer) const {
        return calc_group_id(pointer_traits::get_service_id(global_pointer));
    }

    [[nodiscard]] size_t calc_group_node_id(size_t node_id) const {
        return node_id % m_group_size;
    }

    [[nodiscard]] size_t data_nodes() const noexcept { return m_data_nodes; }

    [[nodiscard]] size_t ec_nodes() const noexcept { return m_ec_nodes; }

private:
    const size_t m_data_nodes;
    const size_t m_ec_nodes;
    const size_t m_group_size;
};
} // namespace uh::cluster

#endif // EC_SCHEME_H
