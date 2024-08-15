//
// Created by massi on 8/14/24.
//

#ifndef EC_GETTER_H
#define EC_GETTER_H
#include "../../utils/ec_scheme.h"
#include "storage/interfaces/storage_system.h"

namespace uh::cluster {

struct ec_get_handler : public service_monitor<storage_system> {
    explicit ec_get_handler(size_t data_nodes, size_t ec_nodes)
        : m_scheme(data_nodes, ec_nodes) {}

    std::shared_ptr<storage_system> get(std::size_t id) const {
        return m_getter.get(m_scheme.get_group_id(id));
    }

    std::shared_ptr<storage_system> get(const uint128_t& pointer) const {
        return get(m_scheme.get_group_id(pointer));
    }

    std::vector<std::shared_ptr<storage_system>> get_services() const {
        return m_getter.get_services();
    }

private:
    void add_client(size_t id,
                    const std::shared_ptr<storage_system>& client) override {
        const auto gid = m_scheme.get_group_id(id);
        if (!m_getter.at(gid).has_value())
            m_getter.add_client(gid, client);
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_system>& client) override {
        if (client->is_empty())
            m_getter.remove_client(id, client);
    }

    ec_scheme m_scheme;
    service_get_handler<storage_system> m_getter;
};
} // namespace uh::cluster

#endif // EC_GETTER_H
