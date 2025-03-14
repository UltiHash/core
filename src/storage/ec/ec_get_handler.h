#pragma once

#include "common/ec/ec_scheme.h"
#include "storage_group.h"

namespace uh::cluster {

struct ec_get_handler : public service_observer<storage_group>,
                        public storage_load_balancer {

    explicit ec_get_handler(
        size_t data_nodes, size_t ec_nodes,
        std::chrono::milliseconds service_get_timeout = SERVICE_GET_TIMEOUT)
        : m_scheme(data_nodes, ec_nodes),
          m_getter{service_get_timeout} {}

    std::shared_ptr<storage_interface> get(std::size_t id) {
        return m_getter.get(m_scheme.calc_group_id(id));
    }

    std::shared_ptr<storage_interface> get(const uint128_t& pointer) {
        return get(m_scheme.calc_group_id(pointer));
    }

    std::vector<std::shared_ptr<storage_interface>> get_services() {
        return m_getter.get_services();
    }

    bool contains(std::size_t id) override {
        return m_getter.contains(m_scheme.calc_group_id(id));
    }

private:
    ec_scheme m_scheme;
    storage_load_balancer m_getter;
};
} // namespace uh::cluster
